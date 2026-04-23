#include "ui/endgame_menu.h"

#include <gtk/gtk.h>

static const char *result_text(GameResult result) {
    switch (result) {
        case RESULT_WHITE_WIN:
            return "White wins.";
        case RESULT_BLACK_WIN:
            return "Black wins.";
        case RESULT_DRAW:
            return "Draw.";
        case RESULT_TERMINATED_BY_USER:
            return "Game ended by user.";
        case RESULT_NONE:
        default:
            return "Game over.";
    }
}

int showEndGameMenu(const GameState *state, int *selection) {
    GtkWidget *dialog;
    int response;

    if (state == NULL || selection == NULL) {
        return 1;
    }

    dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_NONE,
        "%s",
        result_text(state->result));
    gtk_window_set_title(GTK_WINDOW(dialog), "Anteater Chess");
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
        "New Game", 1,
        "Main Menu", 2,
        "Exit", 3,
        NULL);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response < 1 || response > 3) {
        return 1;
    }

    *selection = response;
    return 0;
}
