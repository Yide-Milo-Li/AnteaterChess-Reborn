#include "gui_internal.h"

#include <stdio.h>
#include <string.h>

#include "time/clock.h"
#include "turn/turn_timer.h"

static void set_piece_image_if_changed(GtkWidget *image, const char *icon) {
    const char *currentIcon;
    GdkPixbuf *pixbuf;
    GError *error = NULL;

    if (!GTK_IS_IMAGE(image)) {
        return;
    }

    currentIcon = g_object_get_data(G_OBJECT(image), "piece-icon-path");
    if (icon == NULL) {
        if (currentIcon != NULL) {
            gtk_image_clear(GTK_IMAGE(image));
            g_object_set_data(G_OBJECT(image), "piece-icon-path", NULL);
        }
        return;
    }

    if (currentIcon != NULL && strcmp(currentIcon, icon) == 0) {
        return;
    }

    /* Decode piece SVGs at board-cell size to avoid loading very large source
     * dimensions (for example 4096x4096), which can crash Cairo/GDK. */
    pixbuf = gdk_pixbuf_new_from_file_at_scale(icon, 56, 56, TRUE, &error);
    if (pixbuf == NULL) {
        if (error != NULL) {
            g_warning("Failed to load piece icon '%s': %s", icon, error->message);
            g_error_free(error);
        }
        gtk_image_clear(GTK_IMAGE(image));
        g_object_set_data(G_OBJECT(image), "piece-icon-path", NULL);
        return;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
    g_object_unref(pixbuf);
    g_object_set_data(G_OBJECT(image), "piece-icon-path", (gpointer) icon);
}

static void build_gameplay_sidebar(Gui *gui, GtkWidget *parent) {
    GtkWidget *timeBox;
    GtkWidget *historyLabel;
    GtkWidget *scrolledWindow;
    GtkWidget *enterBox;
    GtkWidget *moveBox;
    GtkWidget *submitButton;
    GtkWidget *undoButton;
    GtkWidget *hintButton;
    GtkWidget *buttonBox;

    timeBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(parent), timeBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timeBox), gtk_label_new("Time Elapsed:"), FALSE, FALSE, 0);
    gui->time_display = gtk_label_new("00:00:00");
    gtk_box_pack_start(GTK_BOX(timeBox), gui->time_display, FALSE, FALSE, 0);

    historyLabel = gtk_label_new("Move History");
    gtk_widget_set_halign(historyLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(parent), historyLabel, FALSE, FALSE, 0);

    gui->history_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gui->history_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gui->history_view), FALSE);
    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, -1, 450);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), gui->history_view);
    gtk_box_pack_start(GTK_BOX(parent), scrolledWindow, FALSE, FALSE, 0);

    gui->status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(gui->status_label), 0.0f);
    gtk_box_pack_start(GTK_BOX(parent), gui->status_label, FALSE, FALSE, 0);

    enterBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(parent), enterBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(enterBox), gtk_label_new("Enter Move"), FALSE, FALSE, 0);

    moveBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(enterBox), moveBox, FALSE, FALSE, 0);
    gui->from_entry = gtk_entry_new();
    gui->to_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(gui->from_entry), "From");
    gtk_entry_set_placeholder_text(GTK_ENTRY(gui->to_entry), "To");
    gtk_box_pack_start(GTK_BOX(moveBox), gui->from_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(moveBox), gui->to_entry, TRUE, TRUE, 0);

    submitButton = gtk_button_new_with_label("Submit");
    gtk_box_pack_start(GTK_BOX(moveBox), submitButton, FALSE, FALSE, 0);
    g_signal_connect(submitButton, "clicked", G_CALLBACK(gui_on_submit_move_clicked), gui);

    undoButton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(undoButton),
        gtk_image_new_from_icon_name("gtk-undo", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_size_request(undoButton, 60, 60);
    g_signal_connect(undoButton, "clicked", G_CALLBACK(gui_on_undo_clicked), gui);

    hintButton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(hintButton),
        gtk_image_new_from_icon_name("gtk-info", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_size_request(hintButton, 60, 60);
    g_signal_connect(hintButton, "clicked", G_CALLBACK(gui_on_hint_clicked), gui);

    buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(buttonBox), undoButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), hintButton, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(parent), buttonBox, TRUE, FALSE, 20);
}

static void build_gameplay_board(Gui *gui, GtkWidget *parent, const GameState *state) {
    GtkWidget *boardBox;
    GtkWidget *rankGrid;
    GtkWidget *boardGrid;
    GtkWidget *fileGrid;
    int row;
    int col;

    gui->black_timer_label = gtk_label_new("Black --:--:--");
    gtk_widget_set_halign(gui->black_timer_label, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(parent), gui->black_timer_label, FALSE, FALSE, 0);

    boardBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(parent), boardBox, TRUE, TRUE, 0);

    rankGrid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(rankGrid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(rankGrid), TRUE);
    gtk_widget_set_size_request(rankGrid, 30, -1);
    gtk_box_pack_start(GTK_BOX(boardBox), rankGrid, FALSE, FALSE, 0);
    for (row = 0; row < 8; ++row) {
        char label[2];

        snprintf(label, sizeof(label), "%d", 8 - row);
        gtk_grid_attach(GTK_GRID(rankGrid), gtk_label_new(label), 0, row, 1, 1);
    }

    boardGrid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(boardGrid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(boardGrid), TRUE);
    gtk_box_pack_start(GTK_BOX(boardBox), boardGrid, TRUE, TRUE, 0);

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            GtkWidget *image = gtk_image_new();
            GtkWidget *eventBox = gtk_event_box_new();
            const char *icon = gui_get_piece_icon(state->board.cells[row][col]);

            set_piece_image_if_changed(image, icon);

            gui->board_images[row][col] = image;
            gtk_container_add(GTK_CONTAINER(eventBox), image);
            gtk_style_context_add_class(gtk_widget_get_style_context(eventBox),
                ((row + col) % 2) == 0 ? "light-square" : "dark-square");
            gtk_grid_attach(GTK_GRID(boardGrid), eventBox, col, row, 1, 1);
        }
    }

    fileGrid = gtk_grid_new();
    gtk_widget_set_size_request(fileGrid, -1, 50);
    gtk_widget_set_hexpand(fileGrid, TRUE);
    gtk_box_pack_start(GTK_BOX(parent), fileGrid, FALSE, FALSE, 0);
    {
        GtkWidget *emptyLabel = gtk_label_new("");
        gtk_widget_set_size_request(emptyLabel, 30, -1);
        gtk_grid_attach(GTK_GRID(fileGrid), emptyLabel, 0, 0, 1, 1);
    }
    for (col = 0; col < 10; ++col) {
        char label[2] = {(char) ('A' + col), '\0'};
        GtkWidget *fileLabel = gtk_label_new(label);

        gtk_widget_set_hexpand(fileLabel, TRUE);
        gtk_grid_attach(GTK_GRID(fileGrid), fileLabel, col + 1, 0, 1, 1);
    }

    gui->white_timer_label = gtk_label_new("White --:--:--");
    gtk_widget_set_halign(gui->white_timer_label, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(parent), gui->white_timer_label, FALSE, FALSE, 0);
}

void gui_build_gameplay_ui(Gui *gui, const GameState *state) {
    GtkWidget *turnLabel;
    GtkWidget *middleBox;
    GtkWidget *leftBox;
    GtkWidget *rightBox;
    GtkWidget *leaveButton;

    gui_rebuild_root_box(gui, GTK_ALIGN_FILL, GTK_ALIGN_FILL, 12);

    turnLabel = gtk_label_new("");
    gtk_widget_set_halign(turnLabel, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), turnLabel, FALSE, FALSE, 0);
    gui->turn_label = turnLabel;

    middleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), middleBox, TRUE, TRUE, 0);

    leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_size_request(leftBox, 400, -1);
    gtk_box_pack_start(GTK_BOX(middleBox), leftBox, FALSE, FALSE, 0);
    build_gameplay_sidebar(gui, leftBox);

    rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(middleBox), rightBox, TRUE, TRUE, 0);
    build_gameplay_board(gui, rightBox, state);

    leaveButton = gtk_button_new_with_label("Leave Game");
    gtk_widget_set_halign(leaveButton, GTK_ALIGN_CENTER);
    gtk_box_pack_end(GTK_BOX(gui->main_box), leaveButton, FALSE, FALSE, 0);
    g_signal_connect(leaveButton, "clicked", G_CALLBACK(gui_on_leave_game_clicked), gui);

    gtk_widget_show_all(gui->window);
}

void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf) {
    if (gui == NULL || row < 0 || row >= 8 || col < 0 || col >= 10) {
        return;
    }

    if (!GTK_IS_IMAGE(gui->board_images[row][col])) {
        return;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(gui->board_images[row][col]), pixbuf);
}

void gui_update_board(Gui *gui, const GameState *state) {
    int row;
    int col;

    if (gui == NULL || state == NULL) {
        return;
    }

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            const char *icon;

            if (!GTK_IS_IMAGE(gui->board_images[row][col])) {
                continue;
            }

            icon = gui_get_piece_icon(state->board.cells[row][col]);
            set_piece_image_if_changed(gui->board_images[row][col], icon);
        }
    }
}

void gui_update_movelist(Gui *gui, const GameState *state) {
    GString *text;
    GtkTextBuffer *buffer;
    int index;

    if (gui == NULL || state == NULL
        || !GTK_IS_WIDGET(gui->history_view) || !GTK_IS_TEXT_VIEW(gui->history_view)) {
        return;
    }

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->history_view));
    text = g_string_new("");

    for (index = 0; index < state->moveHistory.count; ++index) {
        Move move = state->moveHistory.moves[index];
        char fromText[8];
        char toText[8];

        gui_format_position_text(move.from, fromText);
        gui_format_position_text(move.to, toText);
        g_string_append_printf(text, "%d. %s-%s\n", index + 1, fromText, toText);
    }

    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}

void gui_update_clock(Gui *gui) {
    char timeText[32];

    if (gui == NULL || !GTK_IS_WIDGET(gui->time_display) || !GTK_IS_LABEL(gui->time_display)) {
        return;
    }

    gui_format_elapsed_text(timeText, getElapsedTimeSeconds());
    gtk_label_set_text(GTK_LABEL(gui->time_display), timeText);
}

void gui_update_timers(Gui *gui, const GameState *state) {
    char whiteText[32];
    char blackText[32];
    int whiteRemaining;
    int blackRemaining;

    if (gui == NULL || state == NULL) {
        return;
    }

    if (!GTK_IS_WIDGET(gui->white_timer_label) || !GTK_IS_LABEL(gui->white_timer_label)
        || !GTK_IS_WIDGET(gui->black_timer_label) || !GTK_IS_LABEL(gui->black_timer_label)) {
        return;
    }

    if (!state->config.timerEnabled) {
        gtk_label_set_text(GTK_LABEL(gui->white_timer_label), "White --:--:--");
        gtk_label_set_text(GTK_LABEL(gui->black_timer_label), "Black --:--:--");
        return;
    }

    whiteRemaining = getRemainingTime(state, WHITE);
    blackRemaining = getRemainingTime(state, BLACK);
    gui_format_timer_text(whiteText, "White", whiteRemaining);
    gui_format_timer_text(blackText, "Black", blackRemaining);
    gtk_label_set_text(GTK_LABEL(gui->white_timer_label), whiteText);
    gtk_label_set_text(GTK_LABEL(gui->black_timer_label), blackText);
}

void gui_update_turn_display(Gui *gui, Color turn) {
    const char *text;

    if (gui == NULL || !GTK_IS_WIDGET(gui->turn_label) || !GTK_IS_LABEL(gui->turn_label)) {
        return;
    }

    if (turn == WHITE) {
        text = "White's Turn";
    } else if (turn == BLACK) {
        text = "Black's Turn";
    } else {
        text = "Waiting...";
    }

    gtk_label_set_text(GTK_LABEL(gui->turn_label), text);
}
