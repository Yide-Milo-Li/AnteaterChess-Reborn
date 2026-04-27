#include "gui_internal.h"

#include <gdk/gdkkeysyms.h>
#include <stdio.h>

#include "error/error.h"

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

static gboolean gui_on_window_state_event(GtkWidget *widget,
                                          GdkEventWindowState *event,
                                          gpointer userData) {
    Gui *gui = (Gui *)userData;

    (void)widget;
    if (gui == NULL || event == NULL) {
        return FALSE;
    }

    if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0) {
        gui->is_fullscreen =
            (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) ? 1 : 0;
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

    if (gui == NULL || !GTK_IS_BUTTON(gui->fullscreen_button)) {
        return;
    }

    label = gui->is_fullscreen ? "Windowed" : "Fullscreen";
    tooltip = gui->is_fullscreen
        ? "Exit fullscreen mode."
        : "Enter fullscreen mode.";
    gtk_button_set_label(GTK_BUTTON(gui->fullscreen_button), label);
    gtk_widget_set_tooltip_text(gui->fullscreen_button, tooltip);
}

static void gui_update_endgame_turn_display(Gui *gui, const GameState *state) {
    char text[96];

    if (gui == NULL || state == NULL
        || !GTK_IS_WIDGET(gui->turn_label)
        || !GTK_IS_LABEL(gui->turn_label)) {
        return;
    }

    snprintf(text,
        sizeof(text),
        "Game Over - %s",
        gui_game_result_text(state->result));
    gtk_label_set_text(GTK_LABEL(gui->turn_label), text);
}

Gui *gui_create(int *argc, char ***argv) {
    Gui *gui;

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
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->window),
        "app-window");

    initDefaultGameConfig(&gui->pendingConfig);
    initController(&gui->controller, &gui->pendingConfig);
    gui->last_rendered_state = EXIT_STATE;
    gui->last_turn = EMPTY_COLOR;
    gui->last_move_count = 0;
    gui->has_rendered_state = 0;
    gui->has_highlight_from = 0;
    gui->sync_source_id = 0;
    gui->is_fullscreen = 0;
    gui->fullscreen_transition_pending = 0;
    gui->should_quit = 0;

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

    g_free(gui);
}

void gui_set_move_provider(Gui *gui, GuiMoveProvider provider, void *context) {
    if (gui == NULL) {
        return;
    }

    gui->move_provider = provider;
    gui->move_provider_context = context;
}

void gui_set_move_provider_reset(Gui *gui,
                                 GuiMoveProviderReset reset,
                                 void *context) {
    if (gui == NULL) {
        return;
    }

    gui->move_provider_reset = reset;
    gui->move_provider_reset_context = context;
}

void gui_set_hint_provider(Gui *gui, GuiHintProvider provider, void *context) {
    if (gui == NULL) {
        return;
    }

    gui->hint_provider = provider;
    gui->hint_provider_context = context;
}

void gui_attach_move_provider(Gui *gui) {
    (void)gui;
}

void gui_run(Gui *gui) {
    if (!gui_window_is_valid(gui)) {
        return;
    }

    gui_sync_from_controller(gui);
    if (gui->should_quit || !gui_window_is_valid(gui)) {
        return;
    }

    if (gui->sync_source_id == 0) {
        gui->sync_source_id = g_timeout_add(100, gui_on_sync_tick, gui);
    }

    gtk_widget_show_all(gui->window);
    gtk_main();
}

void gui_sync_from_controller(Gui *gui) {
    const GameState *previousState;
    const GameState *state;
    SystemState previousSystemState;
    Color previousTurn;
    int previousMoveCount;
    int previousTimerEnabled;

    if (!gui_window_is_valid(gui)) {
        return;
    }

    previousState = controllerGetState(&gui->controller);
    previousSystemState = previousState != NULL ? previousState->systemState : EXIT_STATE;
    previousTurn = previousState != NULL ? previousState->currentTurn : EMPTY_COLOR;
    previousMoveCount = previousState != NULL ? previousState->moveHistory.count : 0;
    previousTimerEnabled = previousState != NULL ? previousState->config.timerEnabled : 0;

    if (controllerSync(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    state = controllerGetState(&gui->controller);
    if (state == NULL) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    if (!gui->has_rendered_state || gui->last_rendered_state != state->systemState) {
        gui_build_screen_for_state(gui, state);
        gui->last_rendered_state = state->systemState;
        gui->has_rendered_state = 1;
    }

    if (state->systemState == GAMEPLAY_STATE || state->systemState == END_GAME_MENU_STATE) {
        if (previousTimerEnabled
            && previousSystemState == GAMEPLAY_STATE
            && previousMoveCount == state->moveHistory.count
            && previousTurn != EMPTY_COLOR
            && previousTurn != state->currentTurn) {
            gui_set_error(gui, ERR_TIME_UP);
        }
        gui_update_board(gui, state);
        if (gui->last_move_count != state->moveHistory.count) {
            gui_update_movelist(gui, state);
        }
        gui_update_clock(gui);
        gui_update_timers(gui, state);
        if (state->systemState == END_GAME_MENU_STATE) {
            gui_update_endgame_turn_display(gui, state);
        } else {
            gui_update_turn_display(gui, state->currentTurn);
        }
        gui_update_gameplay_controls(gui, state);
        if (state->systemState == GAMEPLAY_STATE) {
            gui_refresh_move_highlights(gui);
            gui_maybe_start_ai_job(gui, state);
        } else {
            gui_clear_move_highlights(gui);
        }
    }

    gui->last_turn = state->currentTurn;
    gui->last_move_count = state->moveHistory.count;
}

int gui_render_snapshot(Gui *gui, const GameState *state) {
    if (gui == NULL || state == NULL) {
        return -1;
    }

    gui->controller.state = *state;
    gui->controller.queue = (EventQueue){0};
    gui_invalidate_async_results(gui);
    gui->pendingConfig = state->config;
    gui->has_rendered_state = 0;
    gui_sync_from_controller(gui);
    return gui->should_quit ? -1 : 0;
}

const GameState *gui_get_state(const Gui *gui) {
    if (gui == NULL) {
        return NULL;
    }

    return controllerGetState(&gui->controller);
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
    Gui *gui = (Gui *) user_data;

    if (gui == NULL || gui->should_quit) {
        return G_SOURCE_REMOVE;
    }

    gui_sync_from_controller(gui);
    if (gui->should_quit) {
        gui->sync_source_id = 0;
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

void gui_on_window_destroy(GtkWidget *widget, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)widget;
    if (gui == NULL) {
        gtk_main_quit();
        return;
    }

    gui->should_quit = 1;
    if (gui->sync_source_id != 0) {
        g_source_remove(gui->sync_source_id);
        gui->sync_source_id = 0;
    }
    gui->window = NULL;
    gui->endgame_dialog = NULL;
    gui_clear_view_refs(gui);
    gtk_main_quit();
}
