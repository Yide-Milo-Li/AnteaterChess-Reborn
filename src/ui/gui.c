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
        ".dark-square { background-color: #b58863; }",
        -1,
        NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    initDefaultGameConfig(&gui->pendingConfig);
    initController(&gui->controller, &gui->pendingConfig);
    gui->last_rendered_state = EXIT_STATE;
    gui->has_rendered_state = 0;
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
    const GameState *state;

    if (!gui_window_is_valid(gui)) {
        return;
    }

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
        gui_update_board(gui, state);
        gui_update_movelist(gui, state);
        gui_update_clock(gui);
        gui_update_timers(gui, state);
        gui_update_turn_display(gui, state->currentTurn);
    }
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
