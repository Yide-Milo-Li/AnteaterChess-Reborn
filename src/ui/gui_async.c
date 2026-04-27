#include "gui_internal.h"

#include <pthread.h>
#include <stdio.h>

/* AI search keeps multiple GameState locals on the stack (each ~200KB due to
 * MoveList[MAX_MOVES]). macOS worker threads default to ~512KB, which is too
 * small — we spawn AI/hint workers on a pthread with an 8MB stack instead. */
#define GUI_ASYNC_THREAD_STACK_BYTES (8 * 1024 * 1024)

typedef enum {
    GUI_ASYNC_AI_MOVE,
    GUI_ASYNC_HINT
} GuiAsyncJobType;

struct GuiAsyncJob {
    Gui *gui;
    GuiAsyncJobType type;
    GameState snapshot;
    GuiMoveProvider moveProvider;
    GuiHintProvider hintProvider;
    void *context;
    Move move;
    int result;
    unsigned int generation;
    guint idleSourceId;
    pthread_t thread;
    int threadStarted;
};

static int state_matches_job_snapshot(const GuiAsyncJob *job) {
    const GameState *state;

    if (job == NULL || job->gui == NULL) {
        return 0;
    }

    state = controllerGetState(&job->gui->controller);
    if (state == NULL) {
        return 0;
    }

    return state->systemState == GAMEPLAY_STATE
        && state->currentTurn == job->snapshot.currentTurn
        && state->moveHistory.count == job->snapshot.moveHistory.count
        && state->moveCount == job->snapshot.moveCount
        && state->config.mode == job->snapshot.config.mode;
}

static void clear_ai_failure_if_state_changed(Gui *gui, const GameState *state) {
    if (gui == NULL || state == NULL || !gui->ai_failure_active) {
        return;
    }

    if (gui->ai_failure_turn != state->currentTurn
        || gui->ai_failure_move_count != state->moveHistory.count) {
        gui->ai_failure_active = 0;
    }
}

static void mark_ai_failure(Gui *gui) {
    const GameState *state;

    if (gui == NULL) {
        return;
    }

    state = controllerGetState(&gui->controller);
    if (state == NULL) {
        return;
    }

    gui->ai_failure_active = 1;
    gui->ai_failure_turn = state->currentTurn;
    gui->ai_failure_move_count = state->moveHistory.count;
}

static void free_job(GuiAsyncJob *job) {
    if (job == NULL) {
        return;
    }

    g_free(job);
}

static void *run_async_job(void *userData) {
    GuiAsyncJob *job = (GuiAsyncJob *)userData;

    if (job == NULL) {
        return NULL;
    }

    if (job->type == GUI_ASYNC_AI_MOVE) {
        job->result = job->moveProvider(&job->snapshot, &job->move, job->context);
    } else {
        job->result = job->hintProvider(&job->snapshot, &job->move, job->context);
    }

    job->idleSourceId = g_idle_add(gui_on_async_job_finished, job);
    return NULL;
}

static int spawn_async_thread(GuiAsyncJob *job, const char *name) {
    pthread_attr_t attr;
    int rc;

    (void)name;
    if (pthread_attr_init(&attr) != 0) {
        return 1;
    }
    if (pthread_attr_setstacksize(&attr, GUI_ASYNC_THREAD_STACK_BYTES) != 0) {
        pthread_attr_destroy(&attr);
        return 1;
    }
    rc = pthread_create(&job->thread, &attr, run_async_job, job);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        return 1;
    }
    job->threadStarted = 1;
    return 0;
}

static void detach_completed_job(GuiAsyncJob *job) {
    Gui *gui;

    if (job == NULL || job->gui == NULL) {
        return;
    }

    gui = job->gui;
    if (job->type == GUI_ASYNC_AI_MOVE) {
        if (gui->ai_job == job) {
            gui->ai_job = NULL;
        }
    } else if (gui->hint_job == job) {
        gui->hint_job = NULL;
    }
}

static int job_is_current(const GuiAsyncJob *job) {
    if (job == NULL || job->gui == NULL) {
        return 0;
    }

    return !job->gui->should_quit
        && job->generation == job->gui->async_generation
        && state_matches_job_snapshot(job);
}

static void complete_ai_job(GuiAsyncJob *job) {
    Gui *gui = job->gui;
    ErrorCode errorCode = ERR_AI_UNAVAILABLE;

    if (!job_is_current(job)) {
        return;
    }

    if (job->result == GUI_MOVE_PROVIDER_TIME_FORFEIT) {
        if (controllerDeclareTimeForfeit(&gui->controller,
                job->snapshot.currentTurn,
                &errorCode) != 0) {
            gui_set_error(gui, errorCode);
            gui_update_gameplay_controls(gui, controllerGetState(&gui->controller));
            return;
        }

        gui->ai_failure_active = 0;
        gui_set_error(gui, ERR_TIME_UP);
        gui_sync_from_controller(gui);
        return;
    }

    if (job->result != 0) {
        mark_ai_failure(gui);
        gui_set_error(gui, ERR_AI_UNAVAILABLE);
        gui_update_gameplay_controls(gui, controllerGetState(&gui->controller));
        return;
    }

    if (controllerSubmitAIMoveDetailed(&gui->controller, job->move, &errorCode) != 0) {
        if (errorCode == ERR_ILLEGAL_MOVE) {
            errorCode = ERR_AI_UNAVAILABLE;
            mark_ai_failure(gui);
        }
        gui_set_error(gui, errorCode);
        gui_update_gameplay_controls(gui, controllerGetState(&gui->controller));
        return;
    }

    gui->ai_failure_active = 0;
    gui_clear_hint_highlight(gui);
    gui_set_status_text(gui, "");
    gui_sync_from_controller(gui);
}

static void complete_hint_job(GuiAsyncJob *job) {
    Gui *gui = job->gui;
    char hintText[64];

    if (!job_is_current(job)) {
        return;
    }

    if (job->result != 0) {
        gui_set_error(gui, ERR_HINT_UNAVAILABLE);
        gui_update_gameplay_controls(gui, controllerGetState(&gui->controller));
        return;
    }

    gui_format_hint_text(job->move, hintText);
    gui_set_status_text(gui, hintText);
    gui_show_hint_move(gui, job->move);
    gui_update_gameplay_controls(gui, controllerGetState(&gui->controller));
}

gboolean gui_on_async_job_finished(gpointer userData) {
    GuiAsyncJob *job = (GuiAsyncJob *)userData;

    if (job == NULL) {
        return G_SOURCE_REMOVE;
    }

    if (job->threadStarted) {
        pthread_join(job->thread, NULL);
        job->threadStarted = 0;
    }

    job->idleSourceId = 0;
    detach_completed_job(job);
    if (job->type == GUI_ASYNC_AI_MOVE) {
        complete_ai_job(job);
    } else {
        complete_hint_job(job);
    }

    free_job(job);
    return G_SOURCE_REMOVE;
}

void gui_invalidate_async_results(Gui *gui) {
    if (gui == NULL) {
        return;
    }

    ++gui->async_generation;
    gui->ai_failure_active = 0;
    gui_clear_hint_highlight(gui);
}

static void cancel_job(GuiAsyncJob **jobSlot) {
    GuiAsyncJob *job;

    if (jobSlot == NULL || *jobSlot == NULL) {
        return;
    }

    job = *jobSlot;
    *jobSlot = NULL;
    if (job->threadStarted) {
        pthread_join(job->thread, NULL);
        job->threadStarted = 0;
    }
    if (job->idleSourceId != 0) {
        g_source_remove(job->idleSourceId);
        job->idleSourceId = 0;
    }
    free_job(job);
}

void gui_cancel_async_jobs(Gui *gui) {
    if (gui == NULL) {
        return;
    }

    gui_invalidate_async_results(gui);
    cancel_job(&gui->ai_job);
    cancel_job(&gui->hint_job);
}

static GuiAsyncJob *create_job(Gui *gui, const GameState *state, GuiAsyncJobType type) {
    GuiAsyncJob *job;

    if (gui == NULL || state == NULL) {
        return NULL;
    }

    job = g_new0(GuiAsyncJob, 1);
    if (job == NULL) {
        return NULL;
    }

    job->gui = gui;
    job->type = type;
    job->snapshot = *state;
    job->moveProvider = gui->move_provider;
    job->hintProvider = gui->hint_provider;
    job->context = (type == GUI_ASYNC_AI_MOVE)
        ? gui->move_provider_context
        : gui->hint_provider_context;
    job->generation = gui->async_generation;
    return job;
}

void gui_maybe_start_ai_job(Gui *gui, const GameState *state) {
    GuiAsyncJob *job;
    char statusText[64];

    if (gui == NULL || state == NULL || gui->move_provider == NULL) {
        return;
    }

    clear_ai_failure_if_state_changed(gui, state);
    if (state->systemState != GAMEPLAY_STATE
        || !gui_current_turn_is_ai(state)
        || gui->ai_job != NULL
        || gui->hint_job != NULL
        || gui->ai_failure_active) {
        return;
    }

    job = create_job(gui, state, GUI_ASYNC_AI_MOVE);
    if (job == NULL) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui->ai_job = job;
    snprintf(statusText, sizeof(statusText), "%s AI thinking...",
        state->currentTurn == WHITE ? "White" : "Black");
    gui_set_status(gui, GUI_STATUS_BUSY, statusText);
    gui_update_gameplay_controls(gui, state);
    if (spawn_async_thread(job, "gui-ai-move") != 0) {
        gui->ai_job = NULL;
        free_job(job);
        gui_set_error(gui, ERR_FATAL);
    }
}

int gui_start_hint_job(Gui *gui) {
    const GameState *state;
    GuiAsyncJob *job;

    if (gui == NULL || gui->hint_provider == NULL
        || gui->ai_job != NULL || gui->hint_job != NULL) {
        return 1;
    }

    state = controllerGetState(&gui->controller);
    if (state == NULL || state->systemState != GAMEPLAY_STATE
        || gui_current_turn_is_ai(state)) {
        return 1;
    }

    job = create_job(gui, state, GUI_ASYNC_HINT);
    if (job == NULL) {
        return 1;
    }

    gui->hint_job = job;
    gui_set_status(gui, GUI_STATUS_BUSY, "Hint thinking...");
    gui_update_gameplay_controls(gui, state);
    if (spawn_async_thread(job, "gui-hint") != 0) {
        gui->hint_job = NULL;
        free_job(job);
        return 1;
    }
    return 0;
}
