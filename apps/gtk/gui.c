#include "gui_internal.h"

#include <gdk/gdkkeysyms.h>
#include <stdio.h>

static gboolean gui_on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer userData) {
    Gui *gui = (Gui *)userData;

    (void)widget;
    if (gui == NULL || event == NULL || !GTK_IS_WINDOW(gui->window)) {
        return FALSE;
    }

    if (event->keyval == GDK_KEY_F11) {
        gui_toggle_fullscreen(gui);
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Escape && gui->is_fullscreen) {
        gui_toggle_fullscreen(gui);
        return TRUE;
    }

    return FALSE;
}

static gboolean gui_on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer userData) {
    Gui *gui = (Gui *)userData;

    (void)widget;
    if (gui == NULL || event == NULL) {
        return FALSE;
    }

    if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0) {
        gui->is_fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) ? 1 : 0;
        gui->fullscreen_transition_pending = 0;
        gui_update_fullscreen_button(gui);
    }
    return FALSE;
}

void gui_toggle_fullscreen(Gui *gui) {
    if (gui == NULL || !GTK_IS_WINDOW(gui->window)) {
        return;
    }
    if (gui->fullscreen_transition_pending) {
        return;
    }

    gui->fullscreen_transition_pending = 1;
    if (gui->is_fullscreen) {
        gtk_window_unfullscreen(GTK_WINDOW(gui->window));
    } else {
        gtk_window_fullscreen(GTK_WINDOW(gui->window));
    }
}

void gui_update_fullscreen_button(Gui *gui) {
    const char *label;
    const char *tooltip;

    if (gui == NULL) {
        return;
    }

    if (GTK_IS_WIDGET(gui->window)) {
        if (gui->is_fullscreen) {
            gtk_style_context_add_class(gtk_widget_get_style_context(gui->window), "fullscreen-mode");
        } else {
            gtk_style_context_remove_class(gtk_widget_get_style_context(gui->window), "fullscreen-mode");
        }
    }

    if (!GTK_IS_BUTTON(gui->fullscreen_button)) {
        return;
    }

    label = gui->is_fullscreen ? "Windowed" : "Fullscreen";
    tooltip = gui->is_fullscreen ? "Exit fullscreen mode." : "Enter fullscreen mode.";
    gtk_button_set_label(GTK_BUTTON(gui->fullscreen_button), label);
    gtk_widget_set_tooltip_text(gui->fullscreen_button, tooltip);
}

static void gui_update_endgame_turn_display(Gui *gui, const GuiView *state) {
    char text[96];

    if (gui == NULL || state == NULL || !GTK_IS_WIDGET(gui->turn_label) || !GTK_IS_LABEL(gui->turn_label)) {
        return;
    }

    snprintf(text, sizeof(text), "Game Over - %s", gui_game_result_text(state->result));
    gtk_label_set_text(GTK_LABEL(gui->turn_label), text);
}

Gui *gui_create(int *argc, char ***argv) {
    Gui *gui;

    ac_platform_prepare_runtime();
    gtk_init(argc, argv);
    gui_install_style();

    gui = g_new0(Gui, 1);
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->window), "Anteater Chess");
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 1200, 840);
    gtk_window_set_resizable(GTK_WINDOW(gui->window), TRUE);
    gtk_window_set_position(GTK_WINDOW(gui->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(gui->window), 24);
    gtk_widget_add_events(gui->window, GDK_KEY_PRESS_MASK | GDK_STRUCTURE_MASK);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->window), "app-window");

    ac_init_default_game_config(&gui->pendingConfig);
    gui->page = AC_MAIN_MENU_STATE;
    gui->log = ac_platform_log_create(NULL);
    AcSessionOptions options = {0};
    options.clock = (AcClock){ac_platform_now, NULL};
    options.log = ac_platform_log_write;
    options.logContext = gui->log;
    gui->session = ac_session_create(&options);
    if (!gui->session) {
        ac_platform_log_destroy(gui->log);
        gtk_widget_destroy(gui->window);
        g_free(gui);
        return NULL;
    }
    gui->last_rendered_state = AC_EXIT_STATE;
    gui->last_turn = AC_EMPTY_COLOR;
    gui->last_move_count = 0;
    gui->has_rendered_state = 0;
    gui->has_highlight_from = 0;
    gui->has_hint_highlight = 0;
    gui->hint_from = ac_create_position(-1, -1);
    gui->hint_to = ac_create_position(-1, -1);
    gui->hint_turn = AC_EMPTY_COLOR;
    gui->hint_move_count = -1;
    gui->sync_source_id = 0;
    gui->is_fullscreen = 0;
    gui->fullscreen_transition_pending = 0;
    gui->should_quit = 0;
    gui->piece_image_size = 56;

    g_signal_connect(gui->window, "destroy", G_CALLBACK(gui_on_window_destroy), gui);
    g_signal_connect(gui->window, "key-press-event", G_CALLBACK(gui_on_window_key_press), gui);
    g_signal_connect(gui->window, "window-state-event", G_CALLBACK(gui_on_window_state_event), gui);
    return gui;
}

void gui_destroy(Gui *gui) {
    if (gui == NULL) {
        return;
    }

    gui->should_quit = 1;
    gui_destroy_endgame_dialog(gui);
    if (gui->sync_source_id != 0) {
        g_source_remove(gui->sync_source_id);
        gui->sync_source_id = 0;
    }
    gui_cancel_async_jobs(gui);

    if (gui_window_is_valid(gui)) {
        gtk_widget_destroy(gui->window);
    }

    ac_session_destroy(gui->session);
    ac_platform_log_destroy(gui->log);
    g_free(gui);
}

void gui_run(Gui *gui) {
    if (!gui_window_is_valid(gui)) {
        return;
    }

    gui_sync(gui);
    if (gui->should_quit || !gui_window_is_valid(gui)) {
        return;
    }

    if (gui->sync_source_id == 0) {
        gui->sync_source_id = g_timeout_add(100, gui_on_sync_tick, gui);
    }

    gtk_widget_show_all(gui->window);
    gtk_main();
}

void gui_sync(Gui *gui) {
    const GuiView *previousState;
    const GuiView *state;
    GuiPage previousSystemState;
    AcColor previousTurn;
    int previousMoveCount;
    int previousTimerEnabled;

    if (!gui_window_is_valid(gui)) {
        return;
    }

    previousState = gui_get_state(gui);
    previousSystemState = previousState != NULL ? previousState->systemState : AC_EXIT_STATE;
    previousTurn = previousState != NULL ? previousState->currentTurn : AC_EMPTY_COLOR;
    previousMoveCount = previousState != NULL ? previousState->moveHistory.count : 0;
    previousTimerEnabled = previousState != NULL ? previousState->config.timerEnabled : 0;

    if (ac_session_tick(gui->session) != 0) {
        gui_set_error(gui, AC_ERR_FATAL);
        return;
    }

    state = gui_get_state(gui);
    if (state == NULL) {
        gui_set_error(gui, AC_ERR_FATAL);
        return;
    }

    if (!gui->has_rendered_state || gui->last_rendered_state != state->systemState) {
        gui_build_screen_for_state(gui, state);
        gui->last_rendered_state = state->systemState;
        gui->has_rendered_state = 1;
    }

    if (state->systemState == AC_GAMEPLAY_STATE || state->systemState == AC_END_GAME_MENU_STATE) {
        if (previousTimerEnabled && previousSystemState == AC_GAMEPLAY_STATE &&
            previousMoveCount == state->moveHistory.count && previousTurn != AC_EMPTY_COLOR &&
            previousTurn != state->currentTurn) {
            gui_set_error(gui, AC_ERR_TIME_UP);
        }
        gui_update_board(gui, state);
        gui_refresh_hint_highlight(gui);
        if (gui->last_move_count != state->moveHistory.count) {
            gui_update_movelist(gui, state);
        }
        gui_update_clock(gui);
        gui_update_timers(gui, state);
        if (state->systemState == AC_END_GAME_MENU_STATE) {
            gui_update_endgame_turn_display(gui, state);
        } else {
            gui_update_turn_display(gui, state->currentTurn);
        }
        gui_update_gameplay_controls(gui, state);
        if (state->systemState == AC_GAMEPLAY_STATE) {
            gui_refresh_move_highlights(gui);
            gui_maybe_start_ai_job(gui, state);
        } else {
            gui_clear_move_highlights(gui);
        }
    }

    if (gui->snapshot.diagnostic != AC_OK)
        gui_set_status(gui, GUI_STATUS_ERROR, ac_status_message(gui->snapshot.diagnostic));
    gui->last_turn = state->currentTurn;
    gui->last_move_count = state->moveHistory.count;
}

const GuiView *gui_get_state(const Gui *value) {
    if (!value)
        return NULL;
    Gui *gui = (Gui *)value;
    ac_session_snapshot(gui->session, &gui->snapshot);
    if (gui->page == AC_GAMEPLAY_STATE && gui->snapshot.phase == AC_SESSION_FINISHED)
        gui->page = AC_END_GAME_MENU_STATE;
    GuiView *v = &gui->view;
    AcSnapshot *s = &gui->snapshot;
    v->position = s->position;
    v->board = s->position.board;
    v->currentTurn = s->position.currentTurn;
    v->config = s->config;
    v->result = s->result;
    v->systemState = gui->page;
    v->revision = s->revision;
    v->moveHistory.moves = s->history;
    v->moveHistory.count = s->historyCount;
    v->moveCount = s->position.moveCount;
    for (int i = 0; i < 2; ++i)
        v->players[i].type = ac_session_is_ai(&s->config, (AcColor)i) ? AC_AI : AC_HUMAN;
    return v;
}

int gui_process_events(void) {
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    return 1;
}

int gui_window_is_valid(const Gui *gui) {
    return gui != NULL && gui->window != NULL && GTK_IS_WIDGET(gui->window);
}

gboolean gui_on_sync_tick(gpointer user_data) {
    Gui *gui = (Gui *)user_data;

    if (gui == NULL || gui->should_quit) {
        return G_SOURCE_REMOVE;
    }

    gui_sync(gui);
    if (gui->should_quit) {
        gui->sync_source_id = 0;
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

void gui_on_window_destroy(GtkWidget *widget, gpointer user_data) {
    Gui *gui = (Gui *)user_data;

    (void)widget;
    if (gui == NULL) {
        if (gtk_main_level() > 0)
            gtk_main_quit();
        return;
    }

    gui->should_quit = 1;
    if (gui->sync_source_id != 0) {
        g_source_remove(gui->sync_source_id);
        gui->sync_source_id = 0;
    }
    gui_invalidate_async_results(gui);
    gui->window = NULL;
    gui->endgame_dialog = NULL;
    gui_clear_view_refs(gui);
    if (gtk_main_level() > 0)
        gtk_main_quit();
}
