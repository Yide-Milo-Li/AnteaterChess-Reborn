#include "gui_internal.h"

#include "error/error.h"
#include "input/command_parser.h"

void gui_on_new_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_quit_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (!gui_confirm(gui, "Quit Game", "Are you sure you want to quit?")) {
        return;
    }

    if (controllerRequestExit(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_mode_selected(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    GameMode mode = (GameMode) GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "game-mode"));

    initGameConfigForMode(&gui->pendingConfig, mode);
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_back_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestBack(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_start_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    GameConfig config;
    ErrorCode errorCode;

    (void)button;
    if (gui_collect_setup_config(gui, &config, &errorCode) != 0) {
        gui_set_error(gui, errorCode);
        return;
    }

    if (controllerStartConfiguredGame(&gui->controller, &config) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui->pendingConfig = config;
    gui_sync_from_controller(gui);
}

void gui_on_setup_timer_toggled(GtkToggleButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    gboolean active;
    int index;

    (void)button;
    if (gui == NULL) {
        return;
    }

    active = GTK_IS_TOGGLE_BUTTON(gui->setup_timer_toggle)
        ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->setup_timer_toggle))
        : FALSE;

    for (index = 0; index < 6; ++index) {
        if (!GTK_IS_WIDGET(gui->setup_timer_widgets[index])) {
            continue;
        }

        if (active) {
            gtk_widget_show(gui->setup_timer_widgets[index]);
        } else {
            gtk_widget_hide(gui->setup_timer_widgets[index]);
        }
    }
}

void gui_on_submit_move_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    const GameState *state;
    const char *fromText;
    const char *toText;
    Command command;

    (void)button;
    if (gui == NULL || !GTK_IS_ENTRY(gui->from_entry) || !GTK_IS_ENTRY(gui->to_entry)) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    state = controllerGetState(&gui->controller);
    if (state == NULL || state->systemState != GAMEPLAY_STATE) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    if (gui_current_turn_is_ai(state)) {
        gui_set_error(gui, ERR_NOT_YOUR_TURN);
        return;
    }

    fromText = gtk_entry_get_text(GTK_ENTRY(gui->from_entry));
    toText = gtk_entry_get_text(GTK_ENTRY(gui->to_entry));
    if (parseMoveCommand(fromText, toText, &command) != 0) {
        gui_set_error(gui, ERR_INVALID_MOVE_FORMAT);
        return;
    }

    if (controllerSubmitMove(&gui->controller, command) != 0) {
        gui_set_error(gui, ERR_ILLEGAL_MOVE);
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(gui->from_entry), "");
    gtk_entry_set_text(GTK_ENTRY(gui->to_entry), "");
    gui_set_status_text(gui, "");
    gui_sync_from_controller(gui);
}

void gui_on_undo_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestUndo(&gui->controller) != 0) {
        gui_set_error(gui, ERR_UNDO_UNAVAILABLE);
        return;
    }

    gui_set_status_text(gui, "");
    gui_sync_from_controller(gui);
}

void gui_on_hint_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    Move move;
    char hintText[64];

    (void)button;
    if (controllerGetHint(&gui->controller, &move) != 0) {
        gui_set_error(gui, ERR_HINT_UNAVAILABLE);
        return;
    }

    gui_format_hint_text(move, hintText);
    gui_set_status_text(gui, hintText);
}

void gui_on_leave_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestLeaveGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_new_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_main_menu_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestBack(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_exit_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestExit(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}
