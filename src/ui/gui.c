#include "gui_internal.h"

#include "error/error.h"

Gui *gui_create(int *argc, char ***argv) {
    GtkCssProvider *provider;
    Gui *gui;

    gtk_init(argc, argv);

    gui = g_new0(Gui, 1);
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->window), "Anteater Chess");
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 1200, 840);
    gtk_window_set_resizable(GTK_WINDOW(gui->window), FALSE);
    gtk_window_set_position(GTK_WINDOW(gui->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(gui->window), 24);

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "GtkWindow { background-color: #2c3e50; } "
        ".light-square { background-color: #f0d9b5; } "
        ".dark-square { background-color: #b58863; } "
        ".highlight-from { background-color: #4f83cc; } "
        ".highlight-destination { background-color: #6abf69; } "
        ".highlight-selected { background-color: #f4d35e; } "
        ".move-input-valid { box-shadow: inset 0 0 0 2px #2e7d32; } "
        ".move-input-invalid { box-shadow: inset 0 0 0 2px #b00020; }",
        -1,
        NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    initDefaultGameConfig(&gui->pendingConfig);
    initController(&gui->controller, &gui->pendingConfig);
    gui->last_rendered_state = EXIT_STATE;
    gui->last_turn = EMPTY_COLOR;
    gui->last_move_count = 0;
    gui->has_rendered_state = 0;
    gui->has_highlight_from = 0;
    gui->endgame_dialog_shown = 0;
    gui->sync_source_id = 0;
    gui->should_quit = 0;

    g_signal_connect(gui->window, "destroy", G_CALLBACK(gui_on_window_destroy), gui);
    return gui;
}

void gui_destroy(Gui *gui) {
    if (gui == NULL) {
        return;
    }

    if (gui->sync_source_id != 0) {
        g_source_remove(gui->sync_source_id);
        gui->sync_source_id = 0;
    }

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
    gui_attach_move_provider(gui);
}

void gui_set_hint_provider(Gui *gui, GuiHintProvider provider, void *context) {
    if (gui == NULL) {
        return;
    }

    gui->hint_provider = provider;
    gui->hint_provider_context = context;
}

void gui_attach_move_provider(Gui *gui) {
    if (gui == NULL) {
        return;
    }

    controllerSetMoveProvider(&gui->controller,
        (ControllerMoveProvider)gui->move_provider,
        gui->move_provider_context);
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

    if (state->systemState == GAMEPLAY_STATE) {
        if (previousTimerEnabled
            && previousSystemState == GAMEPLAY_STATE
            && previousMoveCount == state->moveHistory.count
            && previousTurn != EMPTY_COLOR
            && previousTurn != state->currentTurn) {
            gui_set_error(gui, ERR_TIME_UP);
        }
        gui_update_board(gui, state);
        gui_update_movelist(gui, state);
        gui_update_clock(gui);
        gui_update_timers(gui, state);
        gui_update_turn_display(gui, state->currentTurn);
        gui_update_gameplay_controls(gui, state);
        gui_refresh_move_highlights(gui);
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
    gui_clear_view_refs(gui);
    gtk_main_quit();
}
