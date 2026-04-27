#include "gui_internal.h"

#include <stdio.h>

#include "error/error.h"

void gui_clear_view_refs(Gui *gui) {
    int row;
    int col;
    int index;

    if (gui == NULL) {
        return;
    }

    gui->main_box = NULL;
    gui->new_game_button = NULL;
    gui->quit_game_button = NULL;
    gui->turn_label = NULL;
    gui->time_display = NULL;
    gui->history_view = NULL;
    gui->history_ai_summary_label = NULL;
    gui->black_timer_label = NULL;
    gui->white_timer_label = NULL;
    gui->status_label = NULL;
    gui->from_entry = NULL;
    gui->to_entry = NULL;
    gui->submit_button = NULL;
    gui->undo_button = NULL;
    gui->hint_button = NULL;
    gui->leave_game_button = NULL;
    gui->setup_timer_toggle = NULL;
    gui->setup_hours_spin = NULL;
    gui->setup_minutes_spin = NULL;
    gui->setup_seconds_spin = NULL;
    gui->setup_side_white = NULL;
    gui->setup_side_black = NULL;
    gui->has_highlight_from = 0;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            gui->board_cells[row][col] = NULL;
            gui->board_images[row][col] = NULL;
            gui->board_piece_labels[row][col] = NULL;
            gui->highlight_destinations[row][col] = 0;
        }
    }

    for (index = 0; index < 6; ++index) {
        gui->setup_timer_widgets[index] = NULL;
    }

    for (index = 0; index < GUI_AI_DIFFICULTY_COUNT; ++index) {
        gui->setup_ai_diff_buttons[index] = NULL;
        gui->setup_white_diff_buttons[index] = NULL;
        gui->setup_black_diff_buttons[index] = NULL;
    }
}

void gui_set_status(Gui *gui, GuiStatusKind kind, const char *text) {
    GtkStyleContext *context;
    const char *className;

    if (gui == NULL || !GTK_IS_WIDGET(gui->status_label) || !GTK_IS_LABEL(gui->status_label)) {
        return;
    }

    switch (kind) {
        case GUI_STATUS_BUSY:
            className = "status-busy";
            break;
        case GUI_STATUS_ERROR:
            className = "status-error";
            break;
        case GUI_STATUS_NORMAL:
        default:
            className = "status-normal";
            break;
    }

    context = gtk_widget_get_style_context(gui->status_label);
    gtk_style_context_remove_class(context, "status-error");
    gtk_style_context_remove_class(context, "status-busy");
    gtk_style_context_remove_class(context, "status-normal");
    gtk_style_context_remove_class(context, "status-success");
    gtk_style_context_add_class(context, className);
    gtk_label_set_text(GTK_LABEL(gui->status_label), (text != NULL) ? text : "");
}

void gui_set_status_text(Gui *gui, const char *text) {
    gui_set_status(gui, GUI_STATUS_NORMAL, text);
}

void gui_show_message_dialog(Gui *gui, GtkMessageType type,
                             GtkButtonsType buttons,
                             const char *title,
                             const char *message) {
    GtkWidget *dialog;
    GtkWindow *parent = NULL;

    if (message == NULL) {
        return;
    }

    if (gui_window_is_valid(gui)) {
        parent = GTK_WINDOW(gui->window);
    }

    dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        type,
        buttons,
        "%s",
        message);
    if (title != NULL) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }

    gui_prepare_modal_dialog(gui, dialog);
    (void)gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void gui_prepare_modal_dialog(Gui *gui, GtkWidget *dialog) {
    GtkWindow *window;

    if (!GTK_IS_WINDOW(dialog)) {
        return;
    }

    window = GTK_WINDOW(dialog);
    if (gui_window_is_valid(gui)) {
        gtk_window_set_transient_for(window, GTK_WINDOW(gui->window));
        gtk_window_set_destroy_with_parent(window, TRUE);
    }

    gtk_window_set_modal(window, TRUE);
    gtk_window_set_keep_above(window, TRUE);
    gtk_window_set_type_hint(window, GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_position(window, GTK_WIN_POS_CENTER_ON_PARENT);
}

void gui_destroy_endgame_dialog(Gui *gui) {
    GtkWidget *dialog;

    if (gui == NULL || gui->endgame_dialog == NULL) {
        return;
    }

    dialog = gui->endgame_dialog;
    gui->endgame_dialog = NULL;
    if (GTK_IS_WIDGET(dialog)) {
        gtk_widget_destroy(dialog);
    }
}

int gui_confirm(Gui *gui, const char *title, const char *message) {
    GtkWidget *dialog;
    GtkWindow *parent = NULL;
    int response;

    if (message == NULL) {
        return 0;
    }

    if (gui_window_is_valid(gui)) {
        parent = GTK_WINDOW(gui->window);
    }

    dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_YES_NO,
        "%s",
        message);
    if (title != NULL) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }

    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_NO);
    gui_prepare_modal_dialog(gui, dialog);
    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return response == GTK_RESPONSE_YES;
}

void gui_set_error(Gui *gui, ErrorCode code) {
    const char *message = getErrorMessage(code);

    if (code == ERR_FATAL) {
        gui_show_message_dialog(gui, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Anteater Chess", message);
        if (gui_window_is_valid(gui)) {
            gui_invalidate_async_results(gui);
            initDefaultGameConfig(&gui->pendingConfig);
            initController(&gui->controller, &gui->pendingConfig);
            gui->last_rendered_state = EXIT_STATE;
            gui->has_rendered_state = 0;
            gui_sync_from_controller(gui);
        }
        return;
    }

    if (gui != NULL && GTK_IS_WIDGET(gui->status_label) && GTK_IS_LABEL(gui->status_label)) {
        char errorText[160];

        snprintf(errorText, sizeof(errorText), "Error: %s", message);
        gui_set_status(gui, GUI_STATUS_ERROR, errorText);
        return;
    }

    gui_show_message_dialog(gui, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Anteater Chess", message);
}

void gui_rebuild_root_box(Gui *gui, GtkAlign halign, GtkAlign valign, int spacing) {
    if (!gui_window_is_valid(gui)) {
        return;
    }

    if (gui->main_box != NULL && GTK_IS_WIDGET(gui->main_box)) {
        gtk_widget_destroy(gui->main_box);
    }

    gui_clear_view_refs(gui);
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->main_box),
        "app-root");
    gtk_widget_set_halign(gui->main_box, halign);
    gtk_widget_set_valign(gui->main_box, valign);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);
}

GtkWidget *gui_create_centered_button(const char *label) {
    GtkWidget *button = gtk_button_new_with_label(label);

    gtk_widget_set_size_request(button, 210, 46);
    gtk_widget_set_hexpand(button, TRUE);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    return button;
}
