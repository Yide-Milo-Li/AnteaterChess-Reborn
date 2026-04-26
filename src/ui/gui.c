#include "gui_internal.h"

#include <gdk/gdkkeysyms.h>

#include "error/error.h"

static gboolean gui_on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer userData) {
    Gui *gui = (Gui *)userData;

    (void)widget;
    if (gui == NULL || event == NULL || !GTK_IS_WINDOW(gui->window)) {
        return FALSE;
    }

    if (event->keyval == GDK_KEY_F11) {
        if (gui->fullscreen_transition_pending) {
            return TRUE;
        }

        gui->fullscreen_transition_pending = 1;
        if (gui->is_fullscreen) {
            gtk_window_unfullscreen(GTK_WINDOW(gui->window));
        } else {
            gtk_window_fullscreen(GTK_WINDOW(gui->window));
        }
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Escape && gui->is_fullscreen) {
        if (gui->fullscreen_transition_pending) {
            return TRUE;
        }

        gui->fullscreen_transition_pending = 1;
        gtk_window_unfullscreen(GTK_WINDOW(gui->window));
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
    }
    return FALSE;
}

Gui *gui_create(int *argc, char ***argv) {
    GtkCssProvider *provider;
    Gui *gui;

    gtk_init(argc, argv);

    gui = g_new0(Gui, 1);
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->window), "Anteater Chess");
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 1200, 840);
    gtk_window_set_resizable(GTK_WINDOW(gui->window), TRUE);
    gtk_window_set_position(GTK_WINDOW(gui->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(gui->window), 24);
    gtk_widget_add_events(gui->window, GDK_KEY_PRESS_MASK | GDK_STRUCTURE_MASK);

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "GtkWindow { background-color: #e8edf3; } "
        "label { color: #111827; } "
        "tooltip { background-color: #111827; border: 1px solid #f8fafc; border-radius: 4px; } "
        "tooltip label { color: #f9fafb; padding: 6px; } "
        "button { color: #111827; background-image: none; background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 6px; padding: 6px 10px; } "
        "button label { color: #111827; } "
        "button:hover { background-color: #e0f2fe; border-color: #0284c7; } "
        "button:active { background-color: #bae6fd; } "
        "button:disabled, button:disabled label { color: #6b7280; background-color: #e5e7eb; } "
        "button.primary-button { color: #ffffff; background-color: #2563eb; border-color: #1d4ed8; font-weight: bold; } "
        "button.primary-button label { color: #ffffff; } "
        "button.primary-button:hover { background-color: #1d4ed8; border-color: #1e40af; } "
        "button.primary-button:disabled { color: #dbeafe; background-color: #64748b; border-color: #475569; } "
        "button.primary-button:disabled label { color: #dbeafe; } "
        "button.ai-status-button, button.ai-status-button:disabled { color: #1e3a8a; background-color: #dbeafe; border-color: #93c5fd; font-weight: bold; } "
        "button.ai-status-button label, button.ai-status-button:disabled label { color: #1e3a8a; } "
        "entry { color: #111827; background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 4px; } "
        ".panel { background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 8px; padding: 10px; } "
        ".menu-panel { background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 8px; padding: 18px; } "
        ".panel-title { color: #0f172a; font-weight: bold; } "
        ".clock-text { color: #475569; font-weight: bold; } "
        ".history-panel { background-color: #ffffff; border: 1px solid #cbd5e1; border-radius: 6px; } "
        ".history-view, textview, textview text { color: #0f172a; background-color: #ffffff; font-size: 12px; } "
        ".status-normal, .status-busy, .status-error, .status-success { border-radius: 6px; padding: 6px; } "
        ".status-normal { color: #334155; background-color: transparent; border: 1px solid transparent; } "
        ".status-busy { color: #1e3a8a; background-color: #dbeafe; border: 1px solid #93c5fd; } "
        ".status-success { color: #166534; background-color: #dcfce7; border: 1px solid #86efac; } "
        ".status-error { color: #991b1b; background-color: #fee2e2; border: 1px solid #fecaca; } "
        ".hint-button { background-color: #f8fafc; border-color: #cbd5e1; } "
        ".hint-button:hover { background-color: #eef6ff; border-color: #94a3b8; } "
        ".format-help { background-color: transparent; border-radius: 999px; padding: 4px; } "
        ".format-help:hover { background-color: #e0f2fe; } "
        ".info-icon { color: #2563eb; font-weight: bold; } "
        ".destructive-button { background-color: #fff1f2; border-color: #fb7185; } "
        ".destructive-button:hover { background-color: #ffe4e6; border-color: #e11d48; } "
        ".turn-banner { color: #0f172a; font-size: 18px; font-weight: bold; padding: 6px 12px; border-radius: 8px; background-color: #f8fafc; border: 1px solid #cbd5e1; } "
        ".light-square { background-color: #f0d9b5; } "
        ".dark-square { background-color: #b58863; } "
        ".piece-fallback { color: #111827; font-size: 28px; font-weight: bold; } "
        ".highlight-from { box-shadow: inset 0 0 0 4px rgba(37, 99, 235, 0.85); } "
        ".highlight-destination { box-shadow: inset 0 0 0 4px rgba(22, 163, 74, 0.82); } "
        ".highlight-selected { box-shadow: inset 0 0 0 5px rgba(234, 179, 8, 0.95); } "
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

    if (state->systemState == GAMEPLAY_STATE) {
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
        gui_update_turn_display(gui, state->currentTurn);
        gui_update_gameplay_controls(gui, state);
        gui_refresh_move_highlights(gui);
        gui_maybe_start_ai_job(gui, state);
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
    gui_clear_view_refs(gui);
    gtk_main_quit();
}
