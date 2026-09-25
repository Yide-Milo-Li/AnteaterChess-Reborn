#include "gui_internal.h"
struct GuiAsyncJob {
    Gui *gui;
    AcPosition position;
    AcSearchOptions options;
    AcSearchResult result;
    uint64_t hashes[AC_MAX_MOVES + 1], revision;
    unsigned generation;
    int hint;
    GCancellable *cancel;
};
static int cancelled(void *context) {
    return g_cancellable_is_cancelled(context);
}
static void worker(GTask *task, gpointer source, gpointer data, GCancellable *cancel) {
    (void)source;
    (void)cancel;
    GuiAsyncJob *j = data;
    AcSearchContext *search = ac_search_create();
    j->result.status = search ? ac_search(search, &j->position, &j->options, &j->result) : AC_OUT_OF_MEMORY;
    ac_search_destroy(search);
    g_task_return_boolean(task, TRUE);
}
static void finished(GObject *source, GAsyncResult *result, gpointer data) {
    (void)source;
    (void)result;
    GuiAsyncJob *j = data;
    Gui *g = j->gui;
    if (j->hint)
        g->hint_job = NULL;
    else
        g->ai_job = NULL;
    const GuiView *state = gui_get_state(g);
    int current = !g->should_quit && g->page == AC_GAMEPLAY_STATE && j->generation == g->async_generation &&
                  j->revision == state->revision && !cancelled(j->cancel);
    if (current && j->result.status == AC_OK) {
        if (j->hint) {
            char text[64];
            gui_format_hint_text(j->result.move, text);
            gui_set_status_text(g, text);
            gui_show_hint_move(g, j->result.move);
        } else {
            AcStatus status =
                ac_session_submit_ai(g->session, j->result.move, j->revision, j->options.budgetMs, j->result.elapsedMs);
            if (status != AC_OK && status != AC_STALE_RESULT)
                gui_set_status(g, GUI_STATUS_ERROR, ac_status_message(status));
            else
                gui_set_status_text(g, "");
        }
    } else if (current && j->result.status != AC_CANCELLED) {
        g->ai_failure_active = !j->hint;
        g->ai_failure_turn = state->currentTurn;
        g->ai_failure_move_count = state->moveHistory.count;
        gui_set_status(g, GUI_STATUS_ERROR, ac_status_message(j->result.status));
    }
    g_object_unref(j->cancel);
    g_free(j);
    if (!g->should_quit)
        gui_sync(g);
}
static int start(Gui *g, int hint) {
    if (g->ai_job || g->hint_job || g->should_quit)
        return 1;
    const GuiView *v = gui_get_state(g);
    if (v->systemState != AC_GAMEPLAY_STATE || (hint && gui_current_turn_is_ai(v)))
        return 1;
    GuiAsyncJob *j = g_try_new0(GuiAsyncJob, 1);
    if (!j)
        return 1;
    j->gui = g;
    j->position = v->position;
    j->revision = v->revision;
    j->generation = g->async_generation;
    j->hint = hint;
    j->cancel = g_cancellable_new();
    j->options.clock = (AcClock){ac_platform_now, NULL};
    j->options.cancelled = cancelled;
    j->options.cancelContext = j->cancel;
    AcAIDifficulty d = v->currentTurn == AC_WHITE ? v->config.aiDifficultyWhite : v->config.aiDifficultyBlack;
    j->options.maxDepth = hint ? 8 : ac_search_depth(d);
    j->options.budgetMs =
        hint ? ac_get_ai_time_budget_ms(&v->config, AC_DIFFICULTY_MEDIUM) : ac_session_ai_budget(g->session);
    if (j->options.budgetMs <= 0)
        j->options.budgetMs = 1;
    j->options.hashCount = g->snapshot.historyCount + 1;
    memcpy(j->hashes, g->snapshot.hashes, (size_t)j->options.hashCount * sizeof(uint64_t));
    j->options.hashes = j->hashes;
    if (hint)
        g->hint_job = j;
    else
        g->ai_job = j;
    GTask *task = g_task_new(NULL, j->cancel, finished, j);
    g_task_set_return_on_cancel(task, FALSE);
    g_task_set_task_data(task, j, NULL);
    g_task_run_in_thread(task, worker);
    g_object_unref(task);
    gui_set_status(g, GUI_STATUS_BUSY, hint ? "Hint thinking..." : "AI thinking...");
    return 0;
}
void gui_invalidate_async_results(Gui *g) {
    if (!g)
        return;
    ++g->async_generation;
    g->ai_failure_active = 0;
    gui_clear_hint_highlight(g);
    if (g->ai_job)
        g_cancellable_cancel(g->ai_job->cancel);
    if (g->hint_job)
        g_cancellable_cancel(g->hint_job->cancel);
}
void gui_cancel_async_jobs(Gui *g) {
    gui_invalidate_async_results(g);
    while (g->ai_job || g->hint_job)
        g_main_context_iteration(NULL, TRUE);
}
void gui_maybe_start_ai_job(Gui *g, const GuiView *v) {
    if (g->ai_failure_active &&
        (v->currentTurn != g->ai_failure_turn || v->moveHistory.count != g->ai_failure_move_count))
        g->ai_failure_active = 0;
    if (g->ai_job && g->ai_job->revision != v->revision)
        g_cancellable_cancel(g->ai_job->cancel);
    if (g->hint_job && g->hint_job->revision != v->revision)
        g_cancellable_cancel(g->hint_job->cancel);
    if (v->systemState == AC_GAMEPLAY_STATE && gui_current_turn_is_ai(v) && !g->ai_failure_active)
        start(g, 0);
}
int gui_start_hint_job(Gui *g) {
    return start(g, 1);
}
