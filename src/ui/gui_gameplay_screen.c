#include "gui_internal.h"

#include <stdio.h>

#include "time/clock.h"
#include "turn/turn_timer.h"

#define GUI_PIECE_IMAGE_SIZE 56
#define GUI_UI_ICON_SIZE 18

static const char *gui_special_move_text(SpecialMove type) {
    switch (type) {
        case CASTLING_KINGSIDE:
            return "Castling Kingside";
        case CASTLING_QUEENSIDE:
            return "Castling Queenside";
        case EN_PASSANT:
            return "En Passant";
        case PROMOTION_QUEEN:
            return "Promotion to Queen";
        case PROMOTION_ROOK:
            return "Promotion to Rook";
        case PROMOTION_BISHOP:
            return "Promotion to Bishop";
        case PROMOTION_KNIGHT:
            return "Promotion to Knight";
        case ANTEATER_CAPTURE:
            return "Anteater Capture";
        case NO_SPECIAL_MOVE:
        default:
            return "None";
    }
}

static const char *gui_piece_type_text(PieceType type) {
    switch (type) {
        case ANT:
            return "Ant";
        case ROOK:
            return "Rook";
        case KNIGHT:
            return "Knight";
        case BISHOP:
            return "Bishop";
        case QUEEN:
            return "Queen";
        case KING:
            return "King";
        case ANTEATER:
            return "Anteater";
        case EMPTY_PIECE:
        default:
            return "Unknown";
    }
}

static void gui_format_history_player(const GameState *state, Color color, char buffer[32]) {
    const char *colorText = (color == BLACK) ? "Black" : "White";

    if (state != NULL && (color == WHITE || color == BLACK)
        && state->players[color].type == AI) {
        snprintf(buffer, 32, "%s (AI)", colorText);
        return;
    }

    snprintf(buffer, 32, "%s", colorText);
}

static void build_gameplay_sidebar(Gui *gui, GtkWidget *parent) {
    GtkWidget *historyPanel;
    GtkWidget *historyHeader;
    GtkWidget *historyIcon;
    GtkWidget *historyLabel;
    GtkWidget *scrolledWindow;
    GtkWidget *enterBox;
    GtkWidget *moveBox;
    GtkWidget *formatHelp;
    GtkWidget *formatIcon;
    GtkWidget *submitButton;
    GtkWidget *undoButton;
    GtkWidget *hintButton;
    GtkWidget *buttonBox;

    historyPanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(historyPanel), "panel");
    gtk_box_pack_start(GTK_BOX(parent), historyPanel, TRUE, TRUE, 0);

    historyHeader = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(historyPanel), historyHeader, FALSE, FALSE, 0);
    historyIcon = gui_create_ui_icon("history-svgrepo-com.svg", GUI_UI_ICON_SIZE);
    if (historyIcon != NULL) {
        gtk_box_pack_start(GTK_BOX(historyHeader), historyIcon, FALSE, FALSE, 0);
    }
    historyLabel = gtk_label_new("Move History");
    gtk_style_context_add_class(gtk_widget_get_style_context(historyLabel), "panel-title");
    gtk_widget_set_halign(historyLabel, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(historyHeader), historyLabel, FALSE, FALSE, 0);
    gui->time_display = gtk_label_new("00:00:00");
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->time_display), "clock-text");
    gtk_widget_set_hexpand(gui->time_display, TRUE);
    gtk_widget_set_halign(gui->time_display, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(historyHeader), gui->time_display, TRUE, TRUE, 0);

    gui->history_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gui->history_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gui->history_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(gui->history_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(gui->history_view), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gui->history_view), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(gui->history_view), 8);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(gui->history_view), 8);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(gui->history_view), 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->history_view), "history-view");
    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(scrolledWindow), "history-panel");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, -1, 260);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), gui->history_view);
    gtk_box_pack_start(GTK_BOX(historyPanel), scrolledWindow, TRUE, TRUE, 0);

    gui->status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(gui->status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(gui->status_label), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->status_label), "status-normal");
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
    gtk_widget_set_tooltip_text(gui->from_entry, "Source square, for example E2.");
    gtk_widget_set_tooltip_text(gui->to_entry, "Destination square, for example E4.");
    g_signal_connect(gui->from_entry, "changed", G_CALLBACK(gui_on_move_entry_changed), gui);
    g_signal_connect(gui->to_entry, "changed", G_CALLBACK(gui_on_move_entry_changed), gui);
    gtk_box_pack_start(GTK_BOX(moveBox), gui->from_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(moveBox), gui->to_entry, TRUE, TRUE, 0);

    formatHelp = gtk_event_box_new();
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(formatHelp), TRUE);
    gtk_widget_set_size_request(formatHelp, 28, 28);
    gtk_widget_set_tooltip_text(formatHelp,
        "Move format: E2 to E4. Castling uses king start/end squares. Promotion is selected after submit.");
    gtk_style_context_add_class(gtk_widget_get_style_context(formatHelp), "format-help");
    formatIcon = gui_create_ui_icon("info-icon-svgrepo-com.svg", GUI_UI_ICON_SIZE);
    if (formatIcon == NULL) {
        formatIcon = gtk_label_new("i");
        gtk_style_context_add_class(gtk_widget_get_style_context(formatIcon), "info-icon");
    }
    gtk_container_add(GTK_CONTAINER(formatHelp), formatIcon);
    gtk_box_pack_start(GTK_BOX(moveBox), formatHelp, FALSE, FALSE, 0);

    submitButton = gtk_button_new_with_label("Submit");
    gui->submit_button = submitButton;
    gtk_style_context_add_class(gtk_widget_get_style_context(submitButton), "primary-button");
    gtk_box_pack_start(GTK_BOX(moveBox), submitButton, FALSE, FALSE, 0);
    g_signal_connect(submitButton, "clicked", G_CALLBACK(gui_on_submit_move_clicked), gui);

    undoButton = gtk_button_new_with_label("Undo");
    gui->undo_button = undoButton;
    gtk_widget_set_size_request(undoButton, 92, 44);
    gtk_widget_set_tooltip_text(undoButton, "Undo the previous move.");
    g_signal_connect(undoButton, "clicked", G_CALLBACK(gui_on_undo_clicked), gui);

    hintButton = gtk_button_new_with_label("Hint");
    gui->hint_button = hintButton;
    gui_set_button_icon(hintButton, "bulb-on-svgrepo-com (1).svg", GUI_UI_ICON_SIZE);
    gtk_style_context_add_class(gtk_widget_get_style_context(hintButton), "hint-button");
    gtk_widget_set_size_request(hintButton, 92, 44);
    gtk_widget_set_tooltip_text(hintButton, "Show a suggested move.");
    g_signal_connect(hintButton, "clicked", G_CALLBACK(gui_on_hint_clicked), gui);

    buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(buttonBox), undoButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), hintButton, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(parent), buttonBox, FALSE, FALSE, 0);
}

static void build_gameplay_board(Gui *gui, GtkWidget *parent, const GameState *state) {
    GtkWidget *boardBox;
    GtkWidget *rankGrid;
    GtkWidget *boardGrid;
    GtkWidget *fileGrid;
    int row;
    int col;

    (void)state;
    gui->black_timer_label = gtk_label_new("Black --:--:--");
    gtk_widget_set_halign(gui->black_timer_label, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(parent), gui->black_timer_label, FALSE, FALSE, 0);

    boardBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(boardBox, TRUE);
    gtk_widget_set_vexpand(boardBox, TRUE);
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
    gtk_widget_set_hexpand(boardGrid, TRUE);
    gtk_widget_set_vexpand(boardGrid, TRUE);
    gtk_box_pack_start(GTK_BOX(boardBox), boardGrid, TRUE, TRUE, 0);

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            GtkWidget *image = gtk_image_new();
            GtkWidget *pieceLabel = gtk_label_new("");
            GtkWidget *pieceBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget *eventBox = gtk_event_box_new();

            gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
            gtk_widget_set_halign(pieceLabel, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(pieceLabel, GTK_ALIGN_CENTER);
            gtk_style_context_add_class(gtk_widget_get_style_context(pieceLabel),
                "piece-fallback");
            gtk_box_pack_start(GTK_BOX(pieceBox), image, TRUE, TRUE, 0);
            gtk_box_pack_start(GTK_BOX(pieceBox), pieceLabel, TRUE, TRUE, 0);

            gui->board_images[row][col] = image;
            gui->board_piece_labels[row][col] = pieceLabel;
            gui->board_cells[row][col] = eventBox;
            gtk_widget_set_hexpand(eventBox, TRUE);
            gtk_widget_set_vexpand(eventBox, TRUE);
            g_object_set_data(G_OBJECT(eventBox), "board-row", GINT_TO_POINTER(row));
            g_object_set_data(G_OBJECT(eventBox), "board-col", GINT_TO_POINTER(col));
            gtk_widget_add_events(eventBox, GDK_BUTTON_PRESS_MASK);
            gtk_container_add(GTK_CONTAINER(eventBox), pieceBox);
            gtk_style_context_add_class(gtk_widget_get_style_context(eventBox),
                ((row + col) % 2) == 0 ? "light-square" : "dark-square");
            g_signal_connect(eventBox, "button-press-event",
                G_CALLBACK(gui_on_board_cell_button_press),
                gui);
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
    gtk_style_context_add_class(gtk_widget_get_style_context(turnLabel), "turn-banner");
    gtk_box_pack_start(GTK_BOX(gui->main_box), turnLabel, FALSE, FALSE, 0);
    gui->turn_label = turnLabel;
    gui->last_move_count = -1;

    middleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(middleBox, TRUE);
    gtk_widget_set_vexpand(middleBox, TRUE);
    gtk_box_pack_start(GTK_BOX(gui->main_box), middleBox, TRUE, TRUE, 0);

    leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_size_request(leftBox, 360, -1);
    gtk_widget_set_vexpand(leftBox, TRUE);
    gtk_box_pack_start(GTK_BOX(middleBox), leftBox, FALSE, TRUE, 0);
    build_gameplay_sidebar(gui, leftBox);

    rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(rightBox, TRUE);
    gtk_widget_set_vexpand(rightBox, TRUE);
    gtk_box_pack_start(GTK_BOX(middleBox), rightBox, TRUE, TRUE, 0);
    build_gameplay_board(gui, rightBox, state);

    leaveButton = gtk_button_new_with_label("Leave Game");
    gtk_style_context_add_class(gtk_widget_get_style_context(leaveButton), "destructive-button");
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
    if (GTK_IS_WIDGET(gui->board_piece_labels[row][col])) {
        gtk_widget_hide(gui->board_piece_labels[row][col]);
    }
}

void gui_update_board(Gui *gui, const GameState *state) {
    int row;
    int col;

    if (gui == NULL || state == NULL) {
        return;
    }

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            Piece piece;
            GdkPixbuf *pixbuf;
            char fallbackText[4];

            if (!GTK_IS_IMAGE(gui->board_images[row][col])
                || !GTK_IS_LABEL(gui->board_piece_labels[row][col])) {
                continue;
            }

            piece = state->board.cells[row][col];
            if (!isValidPiece(piece) || piece.type == EMPTY_PIECE) {
                gtk_image_clear(GTK_IMAGE(gui->board_images[row][col]));
                gtk_label_set_text(GTK_LABEL(gui->board_piece_labels[row][col]), "");
                gtk_widget_hide(gui->board_images[row][col]);
                gtk_widget_hide(gui->board_piece_labels[row][col]);
                continue;
            }

            pixbuf = gui_get_piece_pixbuf(piece, GUI_PIECE_IMAGE_SIZE);
            if (pixbuf != NULL) {
                gtk_image_set_from_pixbuf(GTK_IMAGE(gui->board_images[row][col]), pixbuf);
                gtk_widget_show(gui->board_images[row][col]);
                gtk_widget_hide(gui->board_piece_labels[row][col]);
            } else {
                gui_format_piece_fallback_text(piece, fallbackText);
                gtk_image_clear(GTK_IMAGE(gui->board_images[row][col]));
                gtk_label_set_text(GTK_LABEL(gui->board_piece_labels[row][col]), fallbackText);
                gtk_widget_hide(gui->board_images[row][col]);
                gtk_widget_show(gui->board_piece_labels[row][col]);
            }
        }
    }
}

void gui_update_movelist(Gui *gui, const GameState *state) {
    GString *text;
    GtkTextBuffer *buffer;
    GtkTextIter endIter;
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
        char playerText[32];

        gui_format_position_text(move.from, fromText);
        gui_format_position_text(move.to, toText);
        gui_format_history_player(state, move.movedPiece.color, playerText);
        g_string_append_printf(text,
            "[Move %03d] %s | %s %s -> %s",
            index + 1,
            playerText,
            gui_piece_type_text(move.movedPiece.type),
            fromText,
            toText);
        if (move.captureCount > 0) {
            g_string_append_printf(text, " | Captures: %d", move.captureCount);
        }
        if (move.specialType != NO_SPECIAL_MOVE) {
            g_string_append_printf(text, " | Special: %s", gui_special_move_text(move.specialType));
        }
        g_string_append_c(text, '\n');
    }

    gtk_text_buffer_set_text(buffer, text->str, -1);
    if (state->moveHistory.count > 0) {
        GtkTextTagTable *tagTable;
        GtkTextTag *latestTag;
        GtkTextIter lineStart;
        GtkTextIter lineEnd;

        tagTable = gtk_text_buffer_get_tag_table(buffer);
        latestTag = gtk_text_tag_table_lookup(tagTable, "latest-move");
        if (latestTag == NULL) {
            latestTag = gtk_text_buffer_create_tag(buffer,
                "latest-move",
                "background", "#e0f2fe",
                "foreground", "#0f172a",
                NULL);
        }

        gtk_text_buffer_get_iter_at_line(buffer, &lineStart, state->moveHistory.count - 1);
        lineEnd = lineStart;
        gtk_text_iter_forward_to_line_end(&lineEnd);
        gtk_text_buffer_apply_tag(buffer, latestTag, &lineStart, &lineEnd);
    }
    gtk_text_buffer_get_end_iter(buffer, &endIter);
    gtk_text_buffer_place_cursor(buffer, &endIter);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(gui->history_view),
        gtk_text_buffer_get_insert(buffer),
        0.05,
        FALSE,
        0.0,
        1.0);
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

void gui_update_gameplay_controls(Gui *gui, const GameState *state) {
    GtkStyleContext *submitContext;
    gboolean gameplayState;
    gboolean aiTurn;
    gboolean humanTurn;
    gboolean canUseHumanControls;
    const char *submitText;

    if (gui == NULL || state == NULL) {
        return;
    }

    gameplayState = state->systemState == GAMEPLAY_STATE;
    aiTurn = gameplayState && gui_current_turn_is_ai(state);
    humanTurn = gameplayState && !aiTurn;
    canUseHumanControls = humanTurn && gui->ai_job == NULL;
    if (GTK_IS_WIDGET(gui->from_entry)) {
        gtk_widget_set_sensitive(gui->from_entry, canUseHumanControls);
        if (GTK_IS_ENTRY(gui->from_entry)) {
            gtk_entry_set_placeholder_text(GTK_ENTRY(gui->from_entry),
                aiTurn ? "AI turn" : "From");
        }
    }
    if (GTK_IS_WIDGET(gui->to_entry)) {
        gtk_widget_set_sensitive(gui->to_entry, canUseHumanControls);
        if (GTK_IS_ENTRY(gui->to_entry)) {
            gtk_entry_set_placeholder_text(GTK_ENTRY(gui->to_entry),
                aiTurn ? "AI turn" : "To");
        }
    }
    if (GTK_IS_WIDGET(gui->submit_button)) {
        submitContext = gtk_widget_get_style_context(gui->submit_button);
        gtk_style_context_remove_class(submitContext, "primary-button");
        gtk_style_context_remove_class(submitContext, "ai-status-button");
        submitText = "Submit";
        if (aiTurn) {
            submitText = gui->ai_job == NULL ? "AI Playing" : "AI Thinking...";
            gtk_style_context_add_class(submitContext, "ai-status-button");
        } else if (canUseHumanControls) {
            gtk_style_context_add_class(submitContext, "primary-button");
        }
        if (GTK_IS_BUTTON(gui->submit_button)) {
            gtk_button_set_label(GTK_BUTTON(gui->submit_button), submitText);
        }
        gtk_widget_set_sensitive(gui->submit_button, canUseHumanControls);
    }
    if (GTK_IS_WIDGET(gui->undo_button)) {
        gtk_widget_set_sensitive(gui->undo_button, canUseHumanControls);
        gtk_widget_set_tooltip_text(gui->undo_button,
            aiTurn ? "Undo is available on human turns only." : "Undo the previous move.");
    }
    if (GTK_IS_WIDGET(gui->hint_button)) {
        gtk_widget_set_sensitive(gui->hint_button,
            canUseHumanControls && gui->hint_job == NULL);
    }
}
