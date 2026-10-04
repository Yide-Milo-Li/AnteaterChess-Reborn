#include "gui_internal.h"

#include <stdio.h>
#include <stdlib.h>

#define GUI_PIECE_IMAGE_SIZE 56
#define GUI_UI_ICON_SIZE 18
#define GUI_FORMAT_HELP_TEXT                                                                                           \
    "Type source and destination squares, for example E2 to E4.\n"                                                     \
    "Castling uses the king start and end squares, for example F1 to H1.\n"                                            \
    "Promotion choices are selected after Submit."

static const char *gui_special_move_text(AcSpecialMove type) {
    switch (type) {
    case AC_CASTLING_KINGSIDE:
        return "Castling Kingside";
    case AC_CASTLING_QUEENSIDE:
        return "Castling Queenside";
    case AC_EN_PASSANT:
        return "En Passant";
    case AC_PROMOTION_QUEEN:
        return "Promotion to Queen";
    case AC_PROMOTION_ROOK:
        return "Promotion to Rook";
    case AC_PROMOTION_BISHOP:
        return "Promotion to Bishop";
    case AC_PROMOTION_KNIGHT:
        return "Promotion to Knight";
    case AC_ANTEATER_CAPTURE:
        return "Anteater Capture";
    case AC_NO_SPECIAL_MOVE:
    default:
        return "None";
    }
}

static const char *gui_piece_type_text(AcPieceType type) {
    switch (type) {
    case AC_ANT:
        return "Ant";
    case AC_ROOK:
        return "Rook";
    case AC_KNIGHT:
        return "Knight";
    case AC_BISHOP:
        return "Bishop";
    case AC_QUEEN:
        return "Queen";
    case AC_KING:
        return "King";
    case AC_ANTEATER:
        return "Anteater";
    case AC_EMPTY_PIECE:
    default:
        return "Unknown";
    }
}

static void gui_format_history_player(const GuiView *state, AcColor color, char buffer[32]) {
    const char *colorText = (color == AC_BLACK) ? "Black" : "White";

    if (state != NULL && (color == AC_WHITE || color == AC_BLACK) && state->players[color].type == AC_AI) {
        snprintf(buffer, 32, "%s (AI)", colorText);
        return;
    }

    snprintf(buffer, 32, "%s", colorText);
}

static const char *gui_ai_difficulty_text(AcAIDifficulty difficulty) {
    switch (difficulty) {
    case AC_DIFFICULTY_EASY:
        return "Easy";
    case AC_DIFFICULTY_MEDIUM:
        return "Medium";
    case AC_DIFFICULTY_HARD:
        return "Hard";
    case AC_DIFFICULTY_EXPERIMENTAL:
        return "Experimental";
    case AC_DIFFICULTY_TOURNAMENT:
        return "Tournament";
    case AC_DIFFICULTY_NONE:
    default:
        return "None";
    }
}

static void gui_update_history_ai_summary(Gui *gui, const GuiView *state) {
    char text[96];

    if (gui == NULL || state == NULL || !GTK_IS_WIDGET(gui->history_ai_summary_label) ||
        !GTK_IS_LABEL(gui->history_ai_summary_label)) {
        return;
    }

    switch (state->config.mode) {
    case AC_MODE_HUMAN_VS_COMPUTER:
        if (state->config.aiDifficultyWhite != AC_DIFFICULTY_NONE) {
            snprintf(text, sizeof(text), "White AI: %s", gui_ai_difficulty_text(state->config.aiDifficultyWhite));
        } else if (state->config.aiDifficultyBlack != AC_DIFFICULTY_NONE) {
            snprintf(text, sizeof(text), "Black AI: %s", gui_ai_difficulty_text(state->config.aiDifficultyBlack));
        } else {
            gtk_label_set_text(GTK_LABEL(gui->history_ai_summary_label), "");
            gtk_widget_hide(gui->history_ai_summary_label);
            return;
        }
        break;
    case AC_MODE_COMPUTER_VS_COMPUTER:
        snprintf(text, sizeof(text), "White AI: %s | Black AI: %s",
                 gui_ai_difficulty_text(state->config.aiDifficultyWhite),
                 gui_ai_difficulty_text(state->config.aiDifficultyBlack));
        break;
    case AC_MODE_HUMAN_VS_HUMAN:
    default:
        gtk_label_set_text(GTK_LABEL(gui->history_ai_summary_label), "");
        gtk_widget_hide(gui->history_ai_summary_label);
        return;
    }

    gtk_label_set_text(GTK_LABEL(gui->history_ai_summary_label), text);
    gtk_widget_show(gui->history_ai_summary_label);
}

static void gui_show_format_help_popover(GtkWidget *popover) {
    if (!GTK_IS_POPOVER(popover)) {
        return;
    }

    gtk_widget_show_all(popover);
}

static void gui_on_format_help_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    gui_show_format_help_popover(GTK_WIDGET(user_data));
}

static GtkWidget *gui_create_format_help_popover(GtkWidget *relativeTo) {
    GtkWidget *popover;
    GtkWidget *content;
    GtkWidget *title;
    GtkWidget *body;

    popover = gtk_popover_new(relativeTo);
    gtk_popover_set_position(GTK_POPOVER(popover), GTK_POS_BOTTOM);
    gtk_style_context_add_class(gtk_widget_get_style_context(popover), "format-popover-shell");

    content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(content), 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(content), "format-popover");

    title = gtk_label_new("Move Format");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0f);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "format-popover-title");
    gtk_box_pack_start(GTK_BOX(content), title, FALSE, FALSE, 0);

    body = gtk_label_new(GUI_FORMAT_HELP_TEXT);
    gtk_label_set_xalign(GTK_LABEL(body), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(body), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(body), 48);
    gtk_style_context_add_class(gtk_widget_get_style_context(body), "format-popover-body");
    gtk_box_pack_start(GTK_BOX(content), body, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(popover), content);
    return popover;
}

static void build_gameplay_sidebar(Gui *gui, GtkWidget *parent) {
    GtkWidget *historyPanel;
    GtkWidget *historyHeader;
    GtkWidget *historyIcon;
    GtkWidget *historyLabel;
    GtkWidget *aiSummaryLabel;
    GtkWidget *scrolledWindow;
    GtkWidget *enterBox;
    GtkWidget *moveBox;
    GtkWidget *formatHelp;
    GtkWidget *formatIcon;
    GtkWidget *formatPopover;
    GtkWidget *submitButton;
    GtkWidget *undoButton;
    GtkWidget *hintButton;
    GtkWidget *buttonBox;

    historyPanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(historyPanel), "panel");
    gtk_style_context_add_class(gtk_widget_get_style_context(historyPanel), "history-card");
    gtk_box_pack_start(GTK_BOX(parent), historyPanel, TRUE, TRUE, 0);

    historyHeader = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(historyPanel), historyHeader, FALSE, FALSE, 0);
    historyIcon = gui_create_ui_icon("icon-history-dark.svg", GUI_UI_ICON_SIZE);
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

    aiSummaryLabel = gtk_label_new("");
    gui->history_ai_summary_label = aiSummaryLabel;
    gtk_label_set_xalign(GTK_LABEL(aiSummaryLabel), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(aiSummaryLabel), TRUE);
    gtk_widget_set_halign(aiSummaryLabel, GTK_ALIGN_START);
    gtk_widget_set_no_show_all(aiSummaryLabel, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(aiSummaryLabel), "ai-summary");
    gtk_box_pack_start(GTK_BOX(historyPanel), aiSummaryLabel, FALSE, FALSE, 0);
    gtk_widget_hide(aiSummaryLabel);

    gui->history_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(gui->history_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(gui->history_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(gui->history_view), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(gui->history_view), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(gui->history_view), 10);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(gui->history_view), 10);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(gui->history_view), 10);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(gui->history_view), 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->history_view), "history-view");
    scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(scrolledWindow), "history-panel");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolledWindow, -1, 300);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), gui->history_view);
    gtk_box_pack_start(GTK_BOX(historyPanel), scrolledWindow, TRUE, TRUE, 0);

    gui->status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(gui->status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(gui->status_label), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->status_label), "status-normal");
    gtk_box_pack_start(GTK_BOX(parent), gui->status_label, FALSE, FALSE, 0);

    enterBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(enterBox), "panel");
    gtk_style_context_add_class(gtk_widget_get_style_context(enterBox), "move-entry-panel");
    gtk_box_pack_start(GTK_BOX(parent), enterBox, FALSE, FALSE, 0);
    {
        GtkWidget *entryLabel = gtk_label_new("Enter Move");

        gtk_widget_set_halign(entryLabel, GTK_ALIGN_START);
        gtk_style_context_add_class(gtk_widget_get_style_context(entryLabel), "section-label");
        gtk_box_pack_start(GTK_BOX(enterBox), entryLabel, FALSE, FALSE, 0);
    }

    moveBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(enterBox), moveBox, FALSE, FALSE, 0);
    gui->from_entry = gtk_entry_new();
    gui->to_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(gui->from_entry), "From");
    gtk_entry_set_placeholder_text(GTK_ENTRY(gui->to_entry), "To");
    gtk_widget_set_size_request(gui->from_entry, 88, 38);
    gtk_widget_set_size_request(gui->to_entry, 88, 38);
    gtk_widget_set_tooltip_text(gui->from_entry, "Source square, for example E2.");
    gtk_widget_set_tooltip_text(gui->to_entry, "Destination square, for example E4.");
    g_signal_connect(gui->from_entry, "changed", G_CALLBACK(gui_on_move_entry_changed), gui);
    g_signal_connect(gui->to_entry, "changed", G_CALLBACK(gui_on_move_entry_changed), gui);
    gtk_box_pack_start(GTK_BOX(moveBox), gui->from_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(moveBox), gui->to_entry, TRUE, TRUE, 0);

    formatHelp = gtk_button_new();
    gtk_widget_set_can_focus(formatHelp, FALSE);
    gtk_widget_set_size_request(formatHelp, 32, 32);
    gtk_style_context_add_class(gtk_widget_get_style_context(formatHelp), "format-help");
    formatIcon = gui_create_ui_icon("icon-info-dark.svg", GUI_UI_ICON_SIZE);
    if (formatIcon == NULL) {
        formatIcon = gtk_label_new("i");
        gtk_style_context_add_class(gtk_widget_get_style_context(formatIcon), "info-icon");
    }
    gtk_button_set_image(GTK_BUTTON(formatHelp), formatIcon);
    gtk_button_set_always_show_image(GTK_BUTTON(formatHelp), TRUE);
    formatPopover = gui_create_format_help_popover(formatHelp);
    g_object_set_data_full(G_OBJECT(formatHelp), "format-popover", g_object_ref_sink(formatPopover),
                           (GDestroyNotify)gtk_widget_destroy);
    g_signal_connect(formatHelp, "clicked", G_CALLBACK(gui_on_format_help_clicked), formatPopover);
    gtk_box_pack_start(GTK_BOX(moveBox), formatHelp, FALSE, FALSE, 0);

    submitButton = gtk_button_new_with_label("Submit");
    gui->submit_button = submitButton;
    gtk_style_context_add_class(gtk_widget_get_style_context(submitButton), "primary-button");
    gtk_widget_set_size_request(submitButton, 96, 40);
    gtk_box_pack_start(GTK_BOX(moveBox), submitButton, FALSE, FALSE, 0);
    g_signal_connect(submitButton, "clicked", G_CALLBACK(gui_on_submit_move_clicked), gui);

    undoButton = gtk_button_new_with_label("Undo");
    gui->undo_button = undoButton;
    gtk_widget_set_size_request(undoButton, 92, 44);
    gtk_widget_set_tooltip_text(undoButton, "Undo the previous move.");
    g_signal_connect(undoButton, "clicked", G_CALLBACK(gui_on_undo_clicked), gui);

    hintButton = gtk_button_new_with_label("Hint");
    gui->hint_button = hintButton;
    gui_set_button_icon(hintButton, "icon-hint-dark.svg", GUI_UI_ICON_SIZE);
    gtk_style_context_add_class(gtk_widget_get_style_context(hintButton), "hint-button");
    gtk_widget_set_size_request(hintButton, 92, 44);
    gtk_widget_set_tooltip_text(hintButton, "Show a suggested move.");
    g_signal_connect(hintButton, "clicked", G_CALLBACK(gui_on_hint_clicked), gui);

    buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(buttonBox), "action-row");
    gtk_box_pack_start(GTK_BOX(buttonBox), undoButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), hintButton, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(parent), buttonBox, FALSE, FALSE, 0);
}

static void gui_on_board_grid_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data) {
    Gui *gui = (Gui *)user_data;
    const GuiView *state;
    int cellWidth;
    int cellHeight;
    int cellSize;
    int targetSize;

    (void)widget;
    if (gui == NULL || allocation == NULL || allocation->width <= 0 || allocation->height <= 0) {
        return;
    }

    cellWidth = allocation->width / 11;
    cellHeight = allocation->height / 9;
    cellSize = cellWidth < cellHeight ? cellWidth : cellHeight;
    if (cellSize <= 10) {
        return;
    }

    targetSize = (int)(cellSize * 0.84f);
    if (targetSize < 32) {
        targetSize = 32;
    } else if (targetSize > 160) {
        targetSize = 160;
    }

    if (abs(targetSize - gui->piece_image_size) >= 2) {
        gui->piece_image_size = targetSize;
        state = gui_get_state(gui);
        if (state != NULL && (state->systemState == AC_GAMEPLAY_STATE || state->systemState == AC_END_GAME_MENU_STATE)) {
            gui_update_board(gui, state);
        }
    }
}

static void build_gameplay_board(Gui *gui, GtkWidget *parent, const GuiView *state) {
    GtkWidget *boardPanel;
    GtkWidget *boardFrame;
    GtkWidget *boardGrid;
    int row;
    int col;

    (void)state;
    boardPanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_hexpand(boardPanel, TRUE);
    gtk_widget_set_vexpand(boardPanel, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(boardPanel), "board-panel");
    gtk_box_pack_start(GTK_BOX(parent), boardPanel, TRUE, TRUE, 0);

    gui->black_timer_label = gtk_label_new("Black --:--:--");
    gtk_widget_set_halign(gui->black_timer_label, GTK_ALIGN_END);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->black_timer_label), "timer-label");
    gtk_box_pack_start(GTK_BOX(boardPanel), gui->black_timer_label, FALSE, FALSE, 0);

    boardFrame = gtk_aspect_frame_new(NULL, 0.5f, 0.5f, 11.0f / 9.0f, FALSE);
    gtk_widget_set_size_request(boardFrame, 616, 504);
    gtk_widget_set_hexpand(boardFrame, TRUE);
    gtk_widget_set_vexpand(boardFrame, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(boardFrame), "board-frame");
    gtk_box_pack_start(GTK_BOX(boardPanel), boardFrame, TRUE, TRUE, 0);

    boardGrid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(boardGrid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(boardGrid), TRUE);
    gtk_widget_set_hexpand(boardGrid, TRUE);
    gtk_widget_set_vexpand(boardGrid, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(boardGrid), "board-grid");
    gtk_container_add(GTK_CONTAINER(boardFrame), boardGrid);
    g_signal_connect(boardGrid, "size-allocate", G_CALLBACK(gui_on_board_grid_size_allocate), gui);

    for (row = 0; row < 8; ++row) {
        char label[2];
        GtkWidget *rankLabel;

        snprintf(label, sizeof(label), "%d", 8 - row);
        rankLabel = gtk_label_new(label);

        gtk_widget_set_hexpand(rankLabel, TRUE);
        gtk_widget_set_vexpand(rankLabel, TRUE);
        gtk_widget_set_halign(rankLabel, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(rankLabel, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(rankLabel), "coordinate-label");
        gtk_grid_attach(GTK_GRID(boardGrid), rankLabel, 0, row, 1, 1);
    }

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            GtkWidget *image = gtk_image_new();
            GtkWidget *pieceLabel = gtk_label_new("");
            GtkWidget *pieceBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            GtkWidget *eventBox = gtk_event_box_new();

            gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(image, GTK_ALIGN_CENTER);
            gtk_widget_set_size_request(image, 1, 1);
            gtk_widget_set_halign(pieceLabel, GTK_ALIGN_CENTER);
            gtk_widget_set_valign(pieceLabel, GTK_ALIGN_CENTER);
            gtk_style_context_add_class(gtk_widget_get_style_context(pieceLabel), "piece-fallback");
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
            g_signal_connect(eventBox, "button-press-event", G_CALLBACK(gui_on_board_cell_button_press), gui);
            gtk_grid_attach(GTK_GRID(boardGrid), eventBox, col + 1, row, 1, 1);
        }
    }

    {
        GtkWidget *emptyLabel = gtk_label_new("");
        gtk_widget_set_hexpand(emptyLabel, TRUE);
        gtk_widget_set_vexpand(emptyLabel, TRUE);
        gtk_grid_attach(GTK_GRID(boardGrid), emptyLabel, 0, 8, 1, 1);
    }
    for (col = 0; col < 10; ++col) {
        char label[2] = {(char)('A' + col), '\0'};
        GtkWidget *fileLabel = gtk_label_new(label);

        gtk_widget_set_hexpand(fileLabel, TRUE);
        gtk_widget_set_vexpand(fileLabel, TRUE);
        gtk_widget_set_halign(fileLabel, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(fileLabel, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(fileLabel), "coordinate-label");
        gtk_grid_attach(GTK_GRID(boardGrid), fileLabel, col + 1, 8, 1, 1);
    }

    gui->white_timer_label = gtk_label_new("White --:--:--");
    gtk_widget_set_halign(gui->white_timer_label, GTK_ALIGN_END);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->white_timer_label), "timer-label");
    gtk_box_pack_end(GTK_BOX(boardPanel), gui->white_timer_label, FALSE, FALSE, 0);
}

void gui_build_gameplay_ui(Gui *gui, const GuiView *state) {
    GtkWidget *topBar;
    GtkWidget *modeLabel;
    GtkWidget *turnLabel;
    GtkWidget *middleBox;
    GtkWidget *leftBox;
    GtkWidget *rightBox;
    GtkWidget *fullscreenButton;
    GtkWidget *leaveButton;

    gui_rebuild_root_box(gui, GTK_ALIGN_FILL, GTK_ALIGN_FILL, 12);

    topBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_hexpand(topBar, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(topBar), "match-bar");
    gtk_box_pack_start(GTK_BOX(gui->main_box), topBar, FALSE, FALSE, 0);

    modeLabel = gtk_label_new(gui_game_mode_title(state->config.mode));
    gtk_widget_set_halign(modeLabel, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(modeLabel), "match-meta");
    gtk_box_pack_start(GTK_BOX(topBar), modeLabel, FALSE, FALSE, 0);

    turnLabel = gtk_label_new("");
    gtk_widget_set_hexpand(turnLabel, TRUE);
    gtk_widget_set_halign(turnLabel, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(turnLabel), "turn-banner");
    gtk_box_pack_start(GTK_BOX(topBar), turnLabel, TRUE, TRUE, 0);
    gui->turn_label = turnLabel;
    gui->last_move_count = -1;

    fullscreenButton = gtk_button_new_with_label("Fullscreen");
    gui->fullscreen_button = fullscreenButton;
    gtk_style_context_add_class(gtk_widget_get_style_context(fullscreenButton), "fullscreen-button");
    gtk_widget_set_size_request(fullscreenButton, 122, 40);
    gtk_widget_set_halign(fullscreenButton, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(topBar), fullscreenButton, FALSE, FALSE, 0);
    g_signal_connect(fullscreenButton, "clicked", G_CALLBACK(gui_on_fullscreen_clicked), gui);
    gui_update_fullscreen_button(gui);

    leaveButton = gtk_button_new_with_label("Leave Game");
    gui->leave_game_button = leaveButton;
    gtk_style_context_add_class(gtk_widget_get_style_context(leaveButton), "destructive-button");
    gtk_widget_set_size_request(leaveButton, 128, 40);
    gtk_widget_set_halign(leaveButton, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(topBar), leaveButton, FALSE, FALSE, 0);
    g_signal_connect(leaveButton, "clicked", G_CALLBACK(gui_on_leave_game_clicked), gui);

    middleBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
    gtk_widget_set_hexpand(middleBox, TRUE);
    gtk_widget_set_vexpand(middleBox, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(middleBox), "game-shell");
    gtk_box_pack_start(GTK_BOX(gui->main_box), middleBox, TRUE, TRUE, 0);

    leftBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_size_request(leftBox, 350, -1);
    gtk_widget_set_vexpand(leftBox, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(leftBox), "sidebar");
    gtk_box_pack_start(GTK_BOX(middleBox), leftBox, FALSE, TRUE, 0);
    build_gameplay_sidebar(gui, leftBox);

    rightBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(rightBox, TRUE);
    gtk_widget_set_vexpand(rightBox, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(rightBox), "board-area");
    gtk_box_pack_start(GTK_BOX(middleBox), rightBox, TRUE, TRUE, 0);
    build_gameplay_board(gui, rightBox, state);

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

void gui_update_board(Gui *gui, const GuiView *state) {
    int row;
    int col;
    int pieceSize;

    if (gui == NULL || state == NULL) {
        return;
    }

    pieceSize = gui->piece_image_size > 0 ? gui->piece_image_size : GUI_PIECE_IMAGE_SIZE;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            AcPiece piece;
            GdkPixbuf *pixbuf;
            char fallbackText[4];

            if (!GTK_IS_IMAGE(gui->board_images[row][col]) || !GTK_IS_LABEL(gui->board_piece_labels[row][col])) {
                continue;
            }

            piece = state->board.cells[row][col];
            if (!ac_is_valid_piece(piece) || piece.type == AC_EMPTY_PIECE) {
                gtk_image_clear(GTK_IMAGE(gui->board_images[row][col]));
                gtk_label_set_text(GTK_LABEL(gui->board_piece_labels[row][col]), "");
                gtk_widget_hide(gui->board_images[row][col]);
                gtk_widget_hide(gui->board_piece_labels[row][col]);
                continue;
            }

            pixbuf = gui_get_piece_pixbuf(piece, pieceSize);
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

static void gui_format_history_line(const GuiView *state, int index, GString *text) {
    AcMove move;
    char fromText[8];
    char toText[8];
    char playerText[32];

    if (state == NULL || text == NULL || index < 0 || index >= state->moveHistory.count) {
        return;
    }

    move = state->moveHistory.moves[index];
    gui_format_position_text(move.from, fromText);
    gui_format_position_text(move.to, toText);
    gui_format_history_player(state, move.movedPiece.color, playerText);
    g_string_append_printf(text, "[Move %03d] %s | %s %s -> %s", index + 1, playerText,
                           gui_piece_type_text(move.movedPiece.type), fromText, toText);
    if (move.captureCount > 0) {
        g_string_append_printf(text, " | Captures: %d", move.captureCount);
    }
    if (move.specialType != AC_NO_SPECIAL_MOVE) {
        g_string_append_printf(text, " | Special: %s", gui_special_move_text(move.specialType));
    }
    g_string_append_c(text, '\n');
}

static GtkTextTag *gui_get_latest_move_tag(GtkTextBuffer *buffer) {
    GtkTextTagTable *tagTable;
    GtkTextTag *latestTag;

    if (buffer == NULL) {
        return NULL;
    }

    tagTable = gtk_text_buffer_get_tag_table(buffer);
    latestTag = gtk_text_tag_table_lookup(tagTable, "latest-move");
    if (latestTag == NULL) {
        latestTag =
            gtk_text_buffer_create_tag(buffer, "latest-move", "background", "#382914", "foreground", "#fef3c7", NULL);
    }
    return latestTag;
}

static void gui_clear_latest_move_tag(GtkTextBuffer *buffer, GtkTextTag *latestTag) {
    GtkTextIter start;
    GtkTextIter end;

    if (buffer == NULL || latestTag == NULL) {
        return;
    }

    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gtk_text_buffer_remove_tag(buffer, latestTag, &start, &end);
}

static void gui_apply_latest_move_tag(GtkTextBuffer *buffer, int moveCount) {
    GtkTextTag *latestTag;
    GtkTextIter lineStart;
    GtkTextIter lineEnd;

    if (buffer == NULL || moveCount <= 0) {
        return;
    }

    latestTag = gui_get_latest_move_tag(buffer);
    if (latestTag == NULL) {
        return;
    }

    gui_clear_latest_move_tag(buffer, latestTag);
    gtk_text_buffer_get_iter_at_line(buffer, &lineStart, moveCount - 1);
    lineEnd = lineStart;
    gtk_text_iter_forward_to_line_end(&lineEnd);
    gtk_text_buffer_apply_tag(buffer, latestTag, &lineStart, &lineEnd);
}

static void gui_scroll_history_to_end(Gui *gui, GtkTextBuffer *buffer) {
    GtkTextIter endIter;

    if (gui == NULL || buffer == NULL || !GTK_IS_TEXT_VIEW(gui->history_view)) {
        return;
    }

    gtk_text_buffer_get_end_iter(buffer, &endIter);
    gtk_text_buffer_place_cursor(buffer, &endIter);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(gui->history_view), gtk_text_buffer_get_insert(buffer), 0.05, FALSE, 0.0,
                                 1.0);
}

static void gui_rebuild_movelist_buffer(GtkTextBuffer *buffer, const GuiView *state) {
    GString *text;
    int index;

    if (buffer == NULL || state == NULL) {
        return;
    }

    text = g_string_new("");
    for (index = 0; index < state->moveHistory.count; ++index) {
        gui_format_history_line(state, index, text);
    }

    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}

void gui_update_movelist(Gui *gui, const GuiView *state) {
    GtkTextBuffer *buffer;
    GtkTextTag *latestTag;
    int renderedCount;
    int index;

    if (gui == NULL || state == NULL || !GTK_IS_WIDGET(gui->history_view) || !GTK_IS_TEXT_VIEW(gui->history_view)) {
        return;
    }

    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->history_view));
    renderedCount = gui->last_move_count;
    if (renderedCount < 0 || renderedCount > state->moveHistory.count) {
        gui_rebuild_movelist_buffer(buffer, state);
    } else if (renderedCount < state->moveHistory.count) {
        latestTag = gui_get_latest_move_tag(buffer);
        gui_clear_latest_move_tag(buffer, latestTag);
        for (index = renderedCount; index < state->moveHistory.count; ++index) {
            GString *line = g_string_new("");
            GtkTextIter endIter;

            gui_format_history_line(state, index, line);
            gtk_text_buffer_get_end_iter(buffer, &endIter);
            gtk_text_buffer_insert(buffer, &endIter, line->str, -1);
            g_string_free(line, TRUE);
        }
    } else {
        return;
    }

    gui_apply_latest_move_tag(buffer, state->moveHistory.count);
    gui_scroll_history_to_end(gui, buffer);
}

void gui_update_clock(Gui *gui) {
    char timeText[32];

    if (gui == NULL || !GTK_IS_WIDGET(gui->time_display) || !GTK_IS_LABEL(gui->time_display)) {
        return;
    }

    gui_format_elapsed_text(timeText, (gui->snapshot.elapsedMs / 1000));
    gtk_label_set_text(GTK_LABEL(gui->time_display), timeText);
}

void gui_update_timers(Gui *gui, const GuiView *state) {
    char whiteText[32];
    char blackText[32];
    int whiteRemaining;
    int blackRemaining;

    if (gui == NULL || state == NULL) {
        return;
    }

    if (!GTK_IS_WIDGET(gui->white_timer_label) || !GTK_IS_LABEL(gui->white_timer_label) ||
        !GTK_IS_WIDGET(gui->black_timer_label) || !GTK_IS_LABEL(gui->black_timer_label)) {
        return;
    }

    if (!state->config.timerEnabled) {
        gtk_label_set_text(GTK_LABEL(gui->white_timer_label), "White --:--:--");
        gtk_label_set_text(GTK_LABEL(gui->black_timer_label), "Black --:--:--");
        return;
    }

    whiteRemaining = gui->snapshot.remaining[AC_WHITE];
    blackRemaining = gui->snapshot.remaining[AC_BLACK];
    gui_format_timer_text(whiteText, "White", whiteRemaining);
    gui_format_timer_text(blackText, "Black", blackRemaining);
    gtk_label_set_text(GTK_LABEL(gui->white_timer_label), whiteText);
    gtk_label_set_text(GTK_LABEL(gui->black_timer_label), blackText);
}

void gui_update_turn_display(Gui *gui, AcColor turn) {
    const char *text;

    if (gui == NULL || !GTK_IS_WIDGET(gui->turn_label) || !GTK_IS_LABEL(gui->turn_label)) {
        return;
    }

    if (turn == AC_WHITE) {
        text = "White's Turn";
    } else if (turn == AC_BLACK) {
        text = "Black's Turn";
    } else {
        text = "Waiting...";
    }

    gtk_label_set_text(GTK_LABEL(gui->turn_label), text);
}

void gui_update_gameplay_controls(Gui *gui, const GuiView *state) {
    GtkStyleContext *submitContext;
    gboolean gameplayState;
    gboolean aiTurn;
    gboolean humanTurn;
    gboolean canUseHumanControls;
    const char *submitText;

    if (gui == NULL || state == NULL) {
        return;
    }

    gui_update_history_ai_summary(gui, state);

    gameplayState = state->systemState == AC_GAMEPLAY_STATE;
    aiTurn = gameplayState && gui_current_turn_is_ai(state);
    humanTurn = gameplayState && !aiTurn;
    canUseHumanControls = humanTurn && gui->ai_job == NULL;
    if (GTK_IS_WIDGET(gui->from_entry)) {
        gtk_widget_set_sensitive(gui->from_entry, canUseHumanControls);
        if (GTK_IS_ENTRY(gui->from_entry)) {
            gtk_entry_set_placeholder_text(GTK_ENTRY(gui->from_entry), aiTurn ? "AI turn" : "From");
        }
    }
    if (GTK_IS_WIDGET(gui->to_entry)) {
        gtk_widget_set_sensitive(gui->to_entry, canUseHumanControls);
        if (GTK_IS_ENTRY(gui->to_entry)) {
            gtk_entry_set_placeholder_text(GTK_ENTRY(gui->to_entry), aiTurn ? "AI turn" : "To");
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
        gtk_widget_set_sensitive(gui->hint_button, canUseHumanControls && gui->hint_job == NULL);
    }
    if (GTK_IS_WIDGET(gui->leave_game_button)) {
        gtk_widget_set_sensitive(gui->leave_game_button, gameplayState);
    }
}
