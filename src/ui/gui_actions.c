#include "gui_internal.h"

#include "input/command_parser.h"
#include "input/move_request.h"

static int gui_select_promotion_choice(Gui *gui, PromotionChoice *choice) {
    GtkWidget *dialog;
    GtkWindow *parent = NULL;
    int response;

    if (choice == NULL) {
        return 1;
    }

    if (gui_window_is_valid(gui)) {
        parent = GTK_WINDOW(gui->window);
    }

    dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "%s",
        "Choose promotion piece");
    gtk_window_set_title(GTK_WINDOW(dialog), "Promotion");
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Queen", PROMOTION_CHOICE_QUEEN);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Rook", PROMOTION_CHOICE_ROOK);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Bishop", PROMOTION_CHOICE_BISHOP);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Knight", PROMOTION_CHOICE_KNIGHT);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch (response) {
        case PROMOTION_CHOICE_QUEEN:
        case PROMOTION_CHOICE_ROOK:
        case PROMOTION_CHOICE_BISHOP:
        case PROMOTION_CHOICE_KNIGHT:
            *choice = (PromotionChoice)response;
            return 0;
        default:
            return 1;
    }
}

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
    const char *fromText;
    const char *toText;
    Command command;
    MoveRequest request;
    ErrorCode errorCode;
    int needsPromotion;

    (void)button;
    if (gui == NULL || !GTK_IS_ENTRY(gui->from_entry) || !GTK_IS_ENTRY(gui->to_entry)) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    fromText = gtk_entry_get_text(GTK_ENTRY(gui->from_entry));
    toText = gtk_entry_get_text(GTK_ENTRY(gui->to_entry));
    if (parseMoveCommand(fromText, toText, &command) != 0) {
        gui_set_error(gui, ERR_INVALID_MOVE_FORMAT);
        return;
    }

    if (createMoveRequestFromCommand(&request, command) != 0) {
        gui_set_error(gui, ERR_INVALID_MOVE_FORMAT);
        return;
    }

    needsPromotion = 0;
    if (controllerMoveRequestNeedsPromotion(&gui->controller, request, &needsPromotion) == 0
        && needsPromotion) {
        PromotionChoice promotion;

        if (gui_select_promotion_choice(gui, &promotion) != 0) {
            return;
        }
        request.promotion = promotion;
    }

    errorCode = ERR_ILLEGAL_MOVE;
    if (controllerSubmitMoveRequestDetailed(&gui->controller, request, &errorCode) != 0) {
        gui_set_error(gui, errorCode);
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
