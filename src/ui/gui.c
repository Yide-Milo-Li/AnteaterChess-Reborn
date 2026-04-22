

#include <gtk/gtk.h>
#include <time.h>
#include "ui/game_setup_menu.h"
#include "ui/gui.h"
#include "time/clock.h"
#include "ui/board_renderer.h"
#include "turn/turn_timer.h"


// Forward declarations
static void on_back_clicked(GtkButton *button, gpointer user_data);
static void on_leave_game_clicked(GtkButton *button, gpointer user_data);
const char *get_piece_icon(Piece piece);

// Callback for square clicks
static void on_square_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data) {
    int index = GPOINTER_TO_INT(data);
    int row = index / 10;
    int col = index % 10;
    int rank = 8 - row;
    char file = 'A' + col;
    if (event->button == 1) {
        g_print("Left click on square %c%d\n", file, rank);
    } else if (event->button == 3) {
        g_print("Right click on square %c%d\n", file, rank);
    }
}

// main menu setup function
static void setup_quit_confirmation(Gui *gui);
void setup_main_menu(Gui *gui);

static void on_new_game_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    setMainMenuSelection(0);
    g_print("New Game button clicked\n");
}

static void on_quit_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *)user_data;
    setMainMenuSelection(1);
    g_print("Quit Game button clicked\n");
    setup_quit_confirmation(gui);
}


static void on_yes_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    Gui *gui = (Gui *)user_data;
    setQuitChoice(1); // Yes
    if (GTK_IS_WIDGET(gui->window)) {
        gtk_widget_destroy(gui->window);
        gui->window = NULL;
    }
}

static void on_no_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    Gui *gui = (Gui *)user_data;
    setQuitChoice(0); // No
    setup_main_menu(gui);
}

static void on_leave_game_clicked(GtkButton *button, gpointer user_data) {
    setLeaveChoice(1);
    g_print("Leave Game button clicked\n");
}

static void setup_quit_confirmation(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    GtkWidget *label = gtk_label_new("Are you sure you want to quit?");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), label, FALSE, FALSE, 0);

    GtkWidget *yes_button = gtk_button_new_with_label("Yes");
    gtk_widget_set_hexpand(yes_button, TRUE);
    gtk_widget_set_halign(yes_button, GTK_ALIGN_CENTER);
    g_signal_connect(yes_button, "clicked", G_CALLBACK(on_yes_clicked), gui);
    gtk_box_pack_start(GTK_BOX(gui->main_box), yes_button, FALSE, FALSE, 0);

    GtkWidget *no_button = gtk_button_new_with_label("No");
    gtk_widget_set_hexpand(no_button, TRUE);
    gtk_widget_set_halign(no_button, GTK_ALIGN_CENTER);
    g_signal_connect(no_button, "clicked", G_CALLBACK(on_no_clicked), gui);
    gtk_box_pack_start(GTK_BOX(gui->main_box), no_button, FALSE, FALSE, 0);

    gtk_widget_show_all(gui->window);
}

void setup_main_menu(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title), "<span size='xx-large' weight='bold'>Anteater Chess</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), title, FALSE, FALSE, 0);

    gui->new_game_button = gtk_button_new_with_label("New Game");
    gtk_widget_set_hexpand(gui->new_game_button, TRUE);
    gtk_widget_set_halign(gui->new_game_button, GTK_ALIGN_CENTER);
    // Pass NULL as user_data, state is accessed via static pointer
    g_signal_connect(gui->new_game_button, "clicked", G_CALLBACK(on_new_game_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->new_game_button, FALSE, FALSE, 0);

    gui->quit_game_button = gtk_button_new_with_label("Quit Game");
    gtk_widget_set_hexpand(gui->quit_game_button, TRUE);
    gtk_widget_set_halign(gui->quit_game_button, GTK_ALIGN_CENTER);
    g_signal_connect(gui->quit_game_button, "clicked", G_CALLBACK(on_quit_game_clicked), gui);
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->quit_game_button, FALSE, FALSE, 0);

    gtk_widget_show_all(gui->window);
}

Gui *gui_create(int *argc, char ***argv) {
    gtk_init(argc, argv);

    Gui *gui = g_new0(Gui, 1);
    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(gui->window), "Anteater Chess");
    gtk_window_set_default_size(GTK_WINDOW(gui->window), 1200, 840);
    gtk_window_set_resizable(GTK_WINDOW(gui->window), FALSE);
    gtk_window_set_position(GTK_WINDOW(gui->window), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width(GTK_CONTAINER(gui->window), 24);

    // Load CSS for styling
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, 
        "GtkWindow { background-color: #2c3e50; } "
        ".light-square { background-color: #f0d9b5; } "
        ".dark-square { background-color: #b58863; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), 
        GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    // Note: destroy signal connection moved to gui_run


    return gui;
}

void gui_destroy(Gui *gui) {
    if (gui == NULL) {
        return;
    }
    if (gui->window != NULL && GTK_IS_WIDGET(gui->window)) {
        gtk_widget_destroy(gui->window);
        gui->window = NULL;
    }
    g_free(gui);
}

void gui_run(Gui *gui) {
    (void)gui;
    g_signal_connect(gui->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_main();
}

// --- Game Mode Menu Setup ---
static void on_game_mode_selected(GtkButton *button, gpointer user_data) {
    int index = GPOINTER_TO_INT(user_data);
    setGameModeSelection(index);
}

void setup_game_mode_menu(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    GtkWidget *label = gtk_label_new("Game Mode Selection");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), label, FALSE, FALSE, 0);

    const char *button_labels[] = {
        "Human vs. Human",
        "Human vs. Computer",
        "Computer vs. Computer",
        "Back"
    };
    for (int i = 0; i < 4; ++i) {
        GtkWidget *button = gtk_button_new_with_label(button_labels[i]);
        gtk_widget_set_hexpand(button, TRUE);
        gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
        if (i < 3) {
            g_signal_connect(button, "clicked", G_CALLBACK(on_game_mode_selected), GINT_TO_POINTER(i));
        } else {
            g_signal_connect(button, "clicked", G_CALLBACK(on_back_clicked), NULL);
        }
        gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(gui->window);
}

void setup_gameplay_ui(Gui *gui, const GameState *gameState) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    // Top: Turn label
    GtkWidget *turn_label = gtk_label_new("White's Turn"); // Placeholder, should be based on gameState->currentPlayer
    gtk_widget_set_halign(turn_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), turn_label, FALSE, FALSE, 0);
    gui->turn_label = turn_label;

    // Middle: Horizontal box for utility and board
    GtkWidget *middle_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), middle_box, TRUE, TRUE, 0);

    // Left panel: Utility (1/3 width)
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_size_request(left_box, 400, -1);
    gtk_box_pack_start(GTK_BOX(middle_box), left_box, FALSE, FALSE, 0);

    // Time Elapsed
    // Time Elapsed
    GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(left_box), time_box, FALSE, FALSE, 0);
    GtkWidget *time_label = gtk_label_new("Time Elapsed:");
    gtk_box_pack_start(GTK_BOX(time_box), time_label, FALSE, FALSE, 0);
    // Time display in HH:MM:SS format
    int64_t elapsed = getElapsedTimeSeconds();
    int hours = elapsed / 3600;
    int mins = (elapsed % 3600) / 60;
    int secs = elapsed % 60;
    char time_str[20];
    sprintf(time_str, "%02d:%02d:%02d", hours, mins, secs);
    GtkWidget *time_display = gtk_label_new(time_str);
    gtk_box_pack_start(GTK_BOX(time_box), time_display, FALSE, FALSE, 0);
    gui->time_display = time_display;

    // Move History
    GtkWidget *history_label = gtk_label_new("Move History");
    gtk_box_pack_start(GTK_BOX(left_box), history_label, FALSE, FALSE, 0);

    GtkWidget *history_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(history_view), FALSE);
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(history_view));
    gtk_text_buffer_set_text(buffer, "Move 1: e2-e4\nMove 2: e7-e5\n", -1);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrolled_window, -1, 450);
    gtk_container_add(GTK_CONTAINER(scrolled_window), history_view);
    gtk_box_pack_start(GTK_BOX(left_box), scrolled_window, FALSE, FALSE, 0);
    gui->history_view = history_view;

    // Enter Move section
    GtkWidget *enter_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(left_box), enter_box, FALSE, FALSE, 0);

    GtkWidget *enter_label = gtk_label_new("Enter Move");
    gtk_box_pack_start(GTK_BOX(enter_box), enter_label, FALSE, FALSE, 0);

    GtkWidget *move_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(enter_box), move_box, FALSE, FALSE, 0);

    GtkWidget *from_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(from_entry), "From:");
    gtk_box_pack_start(GTK_BOX(move_box), from_entry, TRUE, TRUE, 0);

    GtkWidget *to_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(to_entry), "To:");
    gtk_box_pack_start(GTK_BOX(move_box), to_entry, TRUE, TRUE, 0);

    GtkWidget *submit_button = gtk_button_new_with_label("Submit");
    gtk_box_pack_start(GTK_BOX(move_box), submit_button, FALSE, FALSE, 0);

    // Undo button at bottom of left
    GtkWidget *undo_button = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(undo_button), gtk_image_new_from_icon_name("gtk-undo", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_size_request(undo_button, 60, 60);

    // Hint button underneath undo
    GtkWidget *hint_button = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(hint_button), gtk_image_new_from_icon_name("gtk-info", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_size_request(hint_button, 60, 60);

    // Button box for horizontal layout
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(button_box), undo_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), hint_button, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(left_box), button_box, TRUE, FALSE, 20);

    // Right panel: Board area
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_box_pack_start(GTK_BOX(middle_box), right_box, TRUE, TRUE, 0);

    // Top: Black Timer
    GtkWidget *black_timer_label = gtk_label_new("Black Timer: 00:00");
    gtk_widget_set_halign(black_timer_label, GTK_ALIGN_END);
    gtk_box_pack_start(GTK_BOX(right_box), black_timer_label, FALSE, FALSE, 0);
    gui->black_timer_label = black_timer_label;

    // Middle: Board grid with rank labels
    GtkWidget *board_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(right_box), board_box, TRUE, TRUE, 0);

    // Left: Rank labels (8 to 1)
    GtkWidget *rank_grid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(rank_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(rank_grid), TRUE);
    gtk_widget_set_size_request(rank_grid, 30, -1); // fixed width, height expands
    gtk_box_pack_start(GTK_BOX(board_box), rank_grid, FALSE, FALSE, 0);
    for (int r = 0; r < 8; ++r) {
        int rank_num = 8 - r;
        char label[2];
        sprintf(label, "%d", rank_num);
        GtkWidget *rank_label = gtk_label_new(label);
        gtk_grid_attach(GTK_GRID(rank_grid), rank_label, 0, r, 1, 1);
    }

    // Center: 8x10 grid for pieces
    GtkWidget *board_grid = gtk_grid_new();
    gtk_grid_set_row_homogeneous(GTK_GRID(board_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(board_grid), TRUE);
    gtk_box_pack_start(GTK_BOX(board_box), board_grid, TRUE, TRUE, 0);

    // Create images for each cell (8 rows x 10 columns)
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 10; ++col) {
            GtkWidget *image = gtk_image_new();
            Piece piece = gameState->board.cells[row][col];
            const char *icon = get_piece_icon(piece);
            if (icon) {
                gtk_image_set_from_icon_name(GTK_IMAGE(image), icon, GTK_ICON_SIZE_BUTTON);
            }
            gui->board_images[row][col] = image;

            GtkWidget *eventbox = gtk_event_box_new();
            gtk_container_add(GTK_CONTAINER(eventbox), image);
            gtk_style_context_add_class(gtk_widget_get_style_context(eventbox), 
                (row + col) % 2 == 0 ? "light-square" : "dark-square");
            gtk_grid_attach(GTK_GRID(board_grid), eventbox, col, row, 1, 1);

            // Connect click signals
            g_signal_connect(eventbox, "button-press-event", G_CALLBACK(on_square_clicked), GINT_TO_POINTER(row * 10 + col));
        }
    }

    // Bottom: File labels (A to J)
    GtkWidget *file_grid = gtk_grid_new();
    // Not homogeneous to allow fixed width for empty
    gtk_widget_set_size_request(file_grid, -1, 50); // width expands, fixed height
    gtk_widget_set_hexpand(file_grid, TRUE);
    gtk_box_pack_start(GTK_BOX(right_box), file_grid, FALSE, FALSE, 0);
    // Empty space for rank labels
    GtkWidget *empty_left = gtk_label_new("");
    gtk_widget_set_size_request(empty_left, 30, -1);
    gtk_grid_attach(GTK_GRID(file_grid), empty_left, 0, 0, 1, 1);
    for (int c = 0; c < 10; ++c) {
        char label[2] = {'A' + c, '\0'};
        GtkWidget *file_label = gtk_label_new(label);
        gtk_widget_set_hexpand(file_label, TRUE);
        gtk_grid_attach(GTK_GRID(file_grid), file_label, c + 1, 0, 1, 1);
    }

    // Bottom: White Timer
    GtkWidget *white_timer_label = gtk_label_new("White Timer: 00:00");
    gtk_widget_set_halign(white_timer_label, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(right_box), white_timer_label, FALSE, FALSE, 0);
    gui->white_timer_label = white_timer_label;

    // Bottom: Leave Game button
    GtkWidget *leave_button = gtk_button_new_with_label("Leave Game");
    gtk_widget_set_halign(leave_button, GTK_ALIGN_CENTER);
    gtk_box_pack_end(GTK_BOX(gui->main_box), leave_button, FALSE, FALSE, 0);
    g_signal_connect(leave_button, "clicked", G_CALLBACK(on_leave_game_clicked), NULL);

    gtk_widget_show_all(gui->window);
}

void gui_reset_selections(void) {
    resetMainMenuSelection();
    resetGameModeSelection();
    resetBackButtonClicked();
    resetStartPressed();
    resetLeaveChoice();
}

void gui_process_events(void) {
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
}

int gui_window_is_valid(Gui *gui) {
    return gui && GTK_IS_WIDGET(gui->window);
}

void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf) {
    if (!gui || row < 0 || row >= 8 || col < 0 || col >= 10) return;
    gtk_image_set_from_pixbuf(GTK_IMAGE(gui->board_images[row][col]), pixbuf);
}

// Update functions for gameplay UI
void update_board(Gui *gui, const GameState *state) {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 10; ++col) {
            Piece piece = state->board.cells[row][col];
            const char *icon = get_piece_icon(piece);
            if (icon && GTK_IS_IMAGE(gui->board_images[row][col])) {
                gtk_image_set_from_icon_name(GTK_IMAGE(gui->board_images[row][col]), icon, GTK_ICON_SIZE_BUTTON);
                gtk_widget_queue_draw(gui->board_images[row][col]);
            }
        }
    }
}

void update_movelist(Gui *gui, const GameState *state) {
    if (!GTK_IS_WIDGET(gui->history_view) || !GTK_IS_TEXT_VIEW(gui->history_view)) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(gui->history_view));
    // Clear buffer
    gtk_text_buffer_set_text(buffer, "", -1);
    // Add moves from state->moveHistory
    char text[1024] = "";
    for (int i = 0; i < state->moveHistory.count; ++i) {
        Move m = state->moveHistory.moves[i];
        char from_file = 'a' + m.from.col;
        char from_rank = '8' - m.from.row;
        char to_file = 'a' + m.to.col;
        char to_rank = '8' - m.to.row;
        char move_str[10];
        sprintf(move_str, "%c%c-%c%c\n", from_file, from_rank, to_file, to_rank);
        strcat(text, move_str);
    }
    gtk_text_buffer_set_text(buffer, text, -1);
}

void update_clock(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->time_display) || !GTK_IS_LABEL(gui->time_display)) return;
    int64_t elapsed = getElapsedTimeSeconds();
    int hours = elapsed / 3600;
    int mins = (elapsed % 3600) / 60;
    int secs = elapsed % 60;
    char time_str[20];
    sprintf(time_str, "%02d:%02d:%02d", hours, mins, secs);
    gtk_label_set_text(GTK_LABEL(gui->time_display), time_str);
}

void update_timers(Gui *gui, const GameState *state) {
    if (!GTK_IS_WIDGET(gui->black_timer_label) || !GTK_IS_LABEL(gui->black_timer_label) ||
        !GTK_IS_WIDGET(gui->white_timer_label) || !GTK_IS_LABEL(gui->white_timer_label)) return;
    if (state->config.timerEnabled) {
        updateTurnTimer((GameState *)state);
        int white_rem = getRemainingTime(state, WHITE);
        int black_rem = getRemainingTime(state, BLACK);
        // Format and display
        int w_h = white_rem / 3600;
        int w_m = (white_rem % 3600) / 60;
        int w_s = white_rem % 60;
        char w_str[25];
        sprintf(w_str, "White: %02d:%02d:%02d", w_h, w_m, w_s);
        gtk_label_set_text(GTK_LABEL(gui->white_timer_label), w_str);

        int b_h = black_rem / 3600;
        int b_m = (black_rem % 3600) / 60;
        int b_s = black_rem % 60;
        char b_str[25];
        sprintf(b_str, "Black: %02d:%02d:%02d", b_h, b_m, b_s);
        gtk_label_set_text(GTK_LABEL(gui->black_timer_label), b_str);
    } else {
        gtk_label_set_text(GTK_LABEL(gui->white_timer_label), "White: -- : --");
        gtk_label_set_text(GTK_LABEL(gui->black_timer_label), "Black: -- : --");
    }
}

void gui_display_turn(Gui *gui, Color turn) {
    if (!GTK_IS_WIDGET(gui->turn_label) || !GTK_IS_LABEL(gui->turn_label)) return;
    const char *text = (turn == WHITE) ? "White's Turn" : "Black's Turn";
    gtk_label_set_text(GTK_LABEL(gui->turn_label), text);
}

const char *get_piece_icon(Piece piece) {
    if (!isValidPiece(piece) || piece.type == EMPTY_PIECE) return NULL;
    const char *white_icons[] = {
        "gtk-dialog-info",      // ANT
        "gtk-dialog-warning",   // ROOK
        "gtk-dialog-question",  // KNIGHT
        "gtk-dialog-error",     // BISHOP
        "gtk-dialog-authentication", // QUEEN
        "gtk-dialog-password",  // KING
        "gtk-dialog-info"       // ANTEATER
    };
    const char *black_icons[] = {
        "gtk-dialog-warning",   // ANT
        "gtk-dialog-question",  // ROOK
        "gtk-dialog-error",     // KNIGHT
        "gtk-dialog-authentication", // BISHOP
        "gtk-dialog-password",  // QUEEN
        "gtk-dialog-info",      // KING
        "gtk-dialog-warning"    // ANTEATER
    };
    int index = piece.type - ANT;
    if (index < 0 || index >= 7) return NULL;
    return piece.color == WHITE ? white_icons[index] : black_icons[index];
}

static void on_turn_timer_toggled(GtkToggleButton *toggle, gpointer user_data) {
    GtkWidget **widgets = (GtkWidget **)user_data;
    gboolean active = gtk_toggle_button_get_active(toggle);
    for (int i = 0; i < 6; ++i) {  // 3 labels + 3 combos
        if (active) {
            gtk_widget_show(widgets[i]);
        } else {
            gtk_widget_hide(widgets[i]);
        }
    }
}
static void on_back_clicked(GtkButton *button, gpointer user_data) {
    g_print("Back button clicked\n");
    setBackButtonClicked(1);
}

static void on_side_selected(GtkToggleButton *button, gpointer user_data) {
    if (gtk_toggle_button_get_active(button)) {
        Color color = (Color)user_data;
        GameConfig config;
        getGameSetupConfig(&config);
        setPlayerColor(color);
        if (color == WHITE) {
            // Human white, AI black, set AI difficulty to previous white
            AIDifficulty ai_diff = config.aiDifficultyWhite;
            if (ai_diff == DIFFICULTY_NONE) ai_diff = DIFFICULTY_EASY;
            setAIDifficultyWhite(DIFFICULTY_NONE);
            setAIDifficultyBlack(ai_diff);
        } else {
            // Human black, AI white, set AI difficulty to previous black
            AIDifficulty ai_diff = config.aiDifficultyBlack;
            if (ai_diff == DIFFICULTY_NONE) ai_diff = DIFFICULTY_EASY;
            setAIDifficultyBlack(DIFFICULTY_NONE);
            setAIDifficultyWhite(ai_diff);
        }
    }
}

static void on_difficulty_selected(GtkToggleButton *button, gpointer user_data) {
    if (gtk_toggle_button_get_active(button)) {
        AIDifficulty diff = (AIDifficulty)user_data;
        GameConfig config;
        getGameSetupConfig(&config);
        if (config.mode == MODE_HUMAN_VS_COMPUTER) {
            if (config.playerColor == WHITE) {
                setAIDifficultyBlack(diff);
                setAIDifficultyWhite(DIFFICULTY_NONE);
            } else {
                setAIDifficultyWhite(diff);
                setAIDifficultyBlack(DIFFICULTY_NONE);
            }
        } else {
            // computer vs computer
            int which = (int)(long)g_object_get_data(G_OBJECT(button), "which");
            if (which == 0) {
                setAIDifficultyWhite(diff);
            } else {
                setAIDifficultyBlack(diff);
            }
        }
    }
}

static void on_timer_toggled_config(GtkToggleButton *button, gpointer user_data) {
    GtkSpinButton *minutes_spin = GTK_SPIN_BUTTON(user_data);
    int active = gtk_toggle_button_get_active(button);
    if (active) {
        GameConfig config;
        getGameSetupConfig(&config);
        if (config.initialTimeSeconds == 0) {
            setInitialTimeSeconds(60); // default to 1 minute
            gtk_spin_button_set_value(minutes_spin, 1);
        }
        setTimerEnabled(1);
    } else {
        setTimerEnabled(0);
    }
}

static void on_time_changed(GtkSpinButton *spin, gpointer user_data) {
    // Assume user_data is the hours spin
    GtkSpinButton *hours_spin = GTK_SPIN_BUTTON(user_data);
    GtkSpinButton *minutes_spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(spin), "minutes"));
    GtkSpinButton *seconds_spin = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(spin), "seconds"));
    int hours = gtk_spin_button_get_value_as_int(hours_spin);
    int minutes = gtk_spin_button_get_value_as_int(minutes_spin);
    int seconds = gtk_spin_button_get_value_as_int(seconds_spin);
    int total_seconds = hours * 3600 + minutes * 60 + seconds;
    if (total_seconds == 0) {
        total_seconds = 1;
        gtk_spin_button_set_value(seconds_spin, 1);
    }
    setInitialTimeSeconds(total_seconds);
}

static void on_start_clicked(GtkButton *button, gpointer user_data) {
    g_print("Start button clicked\n");
    setStartPressed(1);
}
void setup_game_setup_menu_human_vs_human(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gui->main_box, GTK_ALIGN_FILL);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    // Content box for the setup options
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(content_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content_box, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(gui->main_box), content_box, TRUE, TRUE, 0);

    GtkWidget *label = gtk_label_new("Human vs. Human");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), label, FALSE, FALSE, 0);

    // Turn timer toggle
    GtkWidget *timer_toggle = gtk_toggle_button_new_with_label("Turn timer");
    gtk_widget_set_halign(timer_toggle, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), timer_toggle, FALSE, FALSE, 0);

    // Timer settings box
    GtkWidget *timer_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(timer_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), timer_box, FALSE, FALSE, 0);

    // Hours spin
    GtkWidget *hours_label = gtk_label_new("Hours:");
    GtkWidget *hours_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_widget_set_size_request(hours_spin, 50, -1);

    // Minutes spin
    GtkWidget *minutes_label = gtk_label_new("Minutes:");
    GtkWidget *minutes_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_widget_set_size_request(minutes_spin, 50, -1);

    // Seconds spin
    GtkWidget *seconds_label = gtk_label_new("Seconds:");
    GtkWidget *seconds_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_widget_set_size_request(seconds_spin, 50, -1);
    g_object_set_data(G_OBJECT(hours_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(hours_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "seconds", seconds_spin);
    g_signal_connect(hours_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(minutes_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(seconds_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_object_set_data(G_OBJECT(hours_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(hours_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "seconds", seconds_spin);
    g_signal_connect(hours_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(minutes_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(seconds_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);

    // Pack into timer_box
    gtk_box_pack_start(GTK_BOX(timer_box), hours_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), hours_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), minutes_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), minutes_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), seconds_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), seconds_spin, FALSE, FALSE, 0);

    // Array of widgets to show/hide
    static GtkWidget *timer_widgets[6];
    timer_widgets[0] = hours_label;
    timer_widgets[1] = hours_spin;
    timer_widgets[2] = minutes_label;
    timer_widgets[3] = minutes_spin;
    timer_widgets[4] = seconds_label;
    timer_widgets[5] = seconds_spin;

    // Initially hide
    for (int i = 0; i < 6; ++i) {
        gtk_widget_hide(timer_widgets[i]);
    }

    // Connect signal
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_turn_timer_toggled), timer_widgets);
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_timer_toggled_config), minutes_spin);

    // Add buttons at the bottom
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), button_box, FALSE, FALSE, 0);

    GtkWidget *back_button = gtk_button_new_with_label("Back");
    gtk_box_pack_start(GTK_BOX(button_box), back_button, FALSE, FALSE, 0);

    GtkWidget *start_button = gtk_button_new_with_label("Start");
    gtk_box_pack_start(GTK_BOX(button_box), start_button, FALSE, FALSE, 0);

    // Connect signals for buttons
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), gui);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), gui);

    gtk_widget_show_all(gui->main_box);
    // Re-hide the timer widgets since show_all showed them
    for (int i = 0; i < 6; ++i) {
        gtk_widget_hide(timer_widgets[i]);
    }
    gtk_widget_show(gui->window);
}

void setup_game_setup_menu_human_vs_computer(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gui->main_box, GTK_ALIGN_FILL);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    // Content box for the setup options
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(content_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content_box, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(gui->main_box), content_box, TRUE, TRUE, 0);

    GtkWidget *label = gtk_label_new("Human vs. Computer");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), label, FALSE, FALSE, 0);

    // Select Side
    GtkWidget *side_label = gtk_label_new("Select Side");
    gtk_widget_set_halign(side_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), side_label, FALSE, FALSE, 0);

    GtkWidget *side_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(side_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), side_box, FALSE, FALSE, 0);

    GtkWidget *white_radio = gtk_radio_button_new_with_label(NULL, "White");
    gtk_box_pack_start(GTK_BOX(side_box), white_radio, FALSE, FALSE, 0);

    GtkWidget *black_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(white_radio), "Black");
    gtk_box_pack_start(GTK_BOX(side_box), black_radio, FALSE, FALSE, 0);

    g_signal_connect(white_radio, "toggled", G_CALLBACK(on_side_selected), (gpointer)WHITE);
    g_signal_connect(black_radio, "toggled", G_CALLBACK(on_side_selected), (gpointer)BLACK);

    // Select Difficulty
    GtkWidget *diff_label = gtk_label_new("Select Difficulty");
    gtk_widget_set_halign(diff_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), diff_label, FALSE, FALSE, 0);

    GtkWidget *diff_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(diff_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), diff_box, FALSE, FALSE, 0);

    GtkWidget *easy_radio = gtk_radio_button_new_with_label(NULL, "Easy");
    gtk_box_pack_start(GTK_BOX(diff_box), easy_radio, FALSE, FALSE, 0);

    GtkWidget *medium_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(easy_radio), "Medium");
    gtk_box_pack_start(GTK_BOX(diff_box), medium_radio, FALSE, FALSE, 0);

    GtkWidget *hard_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(easy_radio), "Hard");
    gtk_box_pack_start(GTK_BOX(diff_box), hard_radio, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(easy_radio), "which", (gpointer)1);
    g_object_set_data(G_OBJECT(medium_radio), "which", (gpointer)1);
    g_object_set_data(G_OBJECT(hard_radio), "which", (gpointer)1);
    g_signal_connect(easy_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_EASY);
    g_signal_connect(medium_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_MEDIUM);
    g_signal_connect(hard_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_HARD);

    // Turn timer toggle
    GtkWidget *timer_toggle = gtk_toggle_button_new_with_label("Turn timer");
    gtk_widget_set_halign(timer_toggle, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), timer_toggle, FALSE, FALSE, 0);

    // Timer settings box
    GtkWidget *timer_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(timer_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), timer_box, FALSE, FALSE, 0);

    // Hours spin
    GtkWidget *hours_label = gtk_label_new("Hours:");
    GtkWidget *hours_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_widget_set_size_request(hours_spin, 50, -1);

    // Minutes spin
    GtkWidget *minutes_label = gtk_label_new("Minutes:");
    GtkWidget *minutes_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_widget_set_size_request(minutes_spin, 50, -1);

    // Seconds spin
    GtkWidget *seconds_label = gtk_label_new("Seconds:");
    GtkWidget *seconds_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_widget_set_size_request(seconds_spin, 50, -1);
    g_object_set_data(G_OBJECT(hours_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(hours_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "seconds", seconds_spin);
    g_signal_connect(hours_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(minutes_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(seconds_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_object_set_data(G_OBJECT(hours_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(hours_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(minutes_spin), "seconds", seconds_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "minutes", minutes_spin);
    g_object_set_data(G_OBJECT(seconds_spin), "seconds", seconds_spin);
    g_signal_connect(hours_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(minutes_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);
    g_signal_connect(seconds_spin, "value-changed", G_CALLBACK(on_time_changed), hours_spin);

    // Pack into timer_box
    gtk_box_pack_start(GTK_BOX(timer_box), hours_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), hours_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), minutes_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), minutes_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), seconds_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timer_box), seconds_spin, FALSE, FALSE, 0);

    // Array of widgets to show/hide
    static GtkWidget *timer_widgets[6];
    timer_widgets[0] = hours_label;
    timer_widgets[1] = hours_spin;
    timer_widgets[2] = minutes_label;
    timer_widgets[3] = minutes_spin;
    timer_widgets[4] = seconds_label;
    timer_widgets[5] = seconds_spin;

    // Initially hide
    for (int i = 0; i < 6; ++i) {
        gtk_widget_hide(timer_widgets[i]);
    }

    // Connect signal
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_turn_timer_toggled), timer_widgets);
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_timer_toggled_config), minutes_spin);

    // Add buttons at the bottom
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), button_box, FALSE, FALSE, 0);

    GtkWidget *back_button = gtk_button_new_with_label("Back");
    gtk_box_pack_start(GTK_BOX(button_box), back_button, FALSE, FALSE, 0);

    GtkWidget *start_button = gtk_button_new_with_label("Start");
    gtk_box_pack_start(GTK_BOX(button_box), start_button, FALSE, FALSE, 0);

    // Connect signals for buttons
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), gui);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), gui);

    gtk_widget_show_all(gui->main_box);
    // Re-hide the timer widgets since show_all showed them
    for (int i = 0; i < 6; ++i) {
        gtk_widget_hide(timer_widgets[i]);
    }
    gtk_widget_show(gui->window);
}

void setup_game_setup_menu_computer_vs_computer(Gui *gui) {
    if (!GTK_IS_WIDGET(gui->window)) return;
    if (gui->main_box) {
        gtk_widget_destroy(gui->main_box);
        gui->main_box = NULL;
    }
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(gui->main_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(gui->main_box, GTK_ALIGN_FILL);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);

    // Content box for the setup options
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(content_box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(content_box, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(gui->main_box), content_box, TRUE, TRUE, 0);

    GtkWidget *label = gtk_label_new("Computer vs. Computer");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), label, FALSE, FALSE, 0);

    // White AI Difficulty
    GtkWidget *white_diff_label = gtk_label_new("White AI Difficulty");
    gtk_widget_set_halign(white_diff_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), white_diff_label, FALSE, FALSE, 0);

    GtkWidget *white_diff_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(white_diff_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), white_diff_box, FALSE, FALSE, 0);

    GtkWidget *white_easy_radio = gtk_radio_button_new_with_label(NULL, "Easy");
    gtk_box_pack_start(GTK_BOX(white_diff_box), white_easy_radio, FALSE, FALSE, 0);

    GtkWidget *white_medium_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(white_easy_radio), "Medium");
    gtk_box_pack_start(GTK_BOX(white_diff_box), white_medium_radio, FALSE, FALSE, 0);

    GtkWidget *white_hard_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(white_easy_radio), "Hard");
    gtk_box_pack_start(GTK_BOX(white_diff_box), white_hard_radio, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(white_easy_radio), "which", (gpointer)0);
    g_object_set_data(G_OBJECT(white_medium_radio), "which", (gpointer)0);
    g_object_set_data(G_OBJECT(white_hard_radio), "which", (gpointer)0);
    g_signal_connect(white_easy_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_EASY);
    g_signal_connect(white_medium_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_MEDIUM);
    g_signal_connect(white_hard_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_HARD);

    // Black AI Difficulty
    GtkWidget *black_diff_label = gtk_label_new("Black AI Difficulty");
    gtk_widget_set_halign(black_diff_label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), black_diff_label, FALSE, FALSE, 0);

    GtkWidget *black_diff_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(black_diff_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(content_box), black_diff_box, FALSE, FALSE, 0);

    GtkWidget *black_easy_radio = gtk_radio_button_new_with_label(NULL, "Easy");
    gtk_box_pack_start(GTK_BOX(black_diff_box), black_easy_radio, FALSE, FALSE, 0);

    GtkWidget *black_medium_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(black_easy_radio), "Medium");
    gtk_box_pack_start(GTK_BOX(black_diff_box), black_medium_radio, FALSE, FALSE, 0);

    GtkWidget *black_hard_radio = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(black_easy_radio), "Hard");
    gtk_box_pack_start(GTK_BOX(black_diff_box), black_hard_radio, FALSE, FALSE, 0);

    g_object_set_data(G_OBJECT(black_easy_radio), "which", (gpointer)1);
    g_object_set_data(G_OBJECT(black_medium_radio), "which", (gpointer)1);
    g_object_set_data(G_OBJECT(black_hard_radio), "which", (gpointer)1);
    g_signal_connect(black_easy_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_EASY);
    g_signal_connect(black_medium_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_MEDIUM);
    g_signal_connect(black_hard_radio, "toggled", G_CALLBACK(on_difficulty_selected), (gpointer)DIFFICULTY_HARD);

    // Add buttons at the bottom
    GtkWidget *button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), button_box, FALSE, FALSE, 0);

    GtkWidget *back_button = gtk_button_new_with_label("Back");
    gtk_box_pack_start(GTK_BOX(button_box), back_button, FALSE, FALSE, 0);

    GtkWidget *start_button = gtk_button_new_with_label("Start");
    gtk_box_pack_start(GTK_BOX(button_box), start_button, FALSE, FALSE, 0);

    // Connect signals for buttons
    g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), gui);
    g_signal_connect(start_button, "clicked", G_CALLBACK(on_start_clicked), gui);

    gtk_widget_show_all(gui->main_box);
    gtk_widget_show(gui->window);
}