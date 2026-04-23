#include "ui/gui.h"

#include <stdio.h>
#include <string.h>

#include "error/error.h"
#include "gameplay/move_resolver.h"
#include "input/command_parser.h"
#include "input/move_request.h"
#include "time/clock.h"
#include "turn/turn_timer.h"

struct Gui {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *new_game_button;
    GtkWidget *quit_game_button;
    GtkWidget *board_images[8][10];
    GtkWidget *turn_label;
    GtkWidget *time_display;
    GtkWidget *history_view;
    GtkWidget *black_timer_label;
    GtkWidget *white_timer_label;
    GtkWidget *status_label;
    GtkWidget *from_entry;
    GtkWidget *to_entry;
    GtkWidget *setup_timer_toggle;
    GtkWidget *setup_hours_spin;
    GtkWidget *setup_minutes_spin;
    GtkWidget *setup_seconds_spin;
    GtkWidget *setup_timer_widgets[6];
    GtkWidget *setup_side_white;
    GtkWidget *setup_side_black;
    GtkWidget *setup_ai_diff_buttons[3];
    GtkWidget *setup_white_diff_buttons[3];
    GtkWidget *setup_black_diff_buttons[3];
    Controller controller;
    GameConfig pendingConfig;
    SystemState last_rendered_state;
    guint sync_source_id;
    int has_rendered_state;
    int should_quit;
};

static gboolean on_sync_tick(gpointer user_data);
static void on_window_destroy(GtkWidget *widget, gpointer user_data);
static void on_new_game_clicked(GtkButton *button, gpointer user_data);
static void on_quit_game_clicked(GtkButton *button, gpointer user_data);
static void on_mode_selected(GtkButton *button, gpointer user_data);
static void on_back_clicked(GtkButton *button, gpointer user_data);
static void on_start_clicked(GtkButton *button, gpointer user_data);
static void on_setup_timer_toggled(GtkToggleButton *button, gpointer user_data);
static void on_submit_move_clicked(GtkButton *button, gpointer user_data);
static void on_undo_clicked(GtkButton *button, gpointer user_data);
static void on_hint_clicked(GtkButton *button, gpointer user_data);
static void on_leave_game_clicked(GtkButton *button, gpointer user_data);
static void on_endgame_new_game_clicked(GtkButton *button, gpointer user_data);
static void on_endgame_main_menu_clicked(GtkButton *button, gpointer user_data);
static void on_endgame_exit_clicked(GtkButton *button, gpointer user_data);

static void gui_clear_view_refs(Gui *gui) {
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
    gui->black_timer_label = NULL;
    gui->white_timer_label = NULL;
    gui->status_label = NULL;
    gui->from_entry = NULL;
    gui->to_entry = NULL;
    gui->setup_timer_toggle = NULL;
    gui->setup_hours_spin = NULL;
    gui->setup_minutes_spin = NULL;
    gui->setup_seconds_spin = NULL;
    gui->setup_side_white = NULL;
    gui->setup_side_black = NULL;

    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            gui->board_images[row][col] = NULL;
        }
    }

    for (index = 0; index < 6; ++index) {
        gui->setup_timer_widgets[index] = NULL;
    }

    for (index = 0; index < 3; ++index) {
        gui->setup_ai_diff_buttons[index] = NULL;
        gui->setup_white_diff_buttons[index] = NULL;
        gui->setup_black_diff_buttons[index] = NULL;
    }
}

static int current_turn_is_ai(const GameState *state) {
    if (state == NULL || state->currentTurn < WHITE || state->currentTurn > BLACK) {
        return 0;
    }

    return state->players[state->currentTurn].type == AI;
}

static const char *game_mode_title(GameMode mode) {
    switch (mode) {
        case MODE_HUMAN_VS_HUMAN:
            return "Human vs. Human";
        case MODE_HUMAN_VS_COMPUTER:
            return "Human vs. Computer";
        case MODE_COMPUTER_VS_COMPUTER:
            return "Computer vs. Computer";
        default:
            return "Game Setup";
    }
}

static const char *game_result_text(GameResult result) {
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

static AIDifficulty difficulty_from_index(int index) {
    switch (index) {
        case 0:
            return DIFFICULTY_EASY;
        case 1:
            return DIFFICULTY_MEDIUM;
        case 2:
            return DIFFICULTY_HARD;
        default:
            return DIFFICULTY_EASY;
    }
}

static int difficulty_index(AIDifficulty difficulty) {
    switch (difficulty) {
        case DIFFICULTY_MEDIUM:
            return 1;
        case DIFFICULTY_HARD:
            return 2;
        case DIFFICULTY_EASY:
        case DIFFICULTY_NONE:
        default:
            return 0;
    }
}

static void format_elapsed_text(char buffer[32], int64_t elapsedSeconds) {
    int hours;
    int minutes;
    int seconds;

    if (buffer == NULL) {
        return;
    }

    if (elapsedSeconds < 0) {
        elapsedSeconds = 0;
    }

    hours = (int) (elapsedSeconds / 3600);
    minutes = (int) ((elapsedSeconds % 3600) / 60);
    seconds = (int) (elapsedSeconds % 60);
    snprintf(buffer, 32, "%02d:%02d:%02d", hours, minutes, seconds);
}

static void format_timer_text(char buffer[32], const char *prefix, int seconds) {
    int hours;
    int minutes;
    int remainderSeconds;

    if (buffer == NULL || prefix == NULL) {
        return;
    }

    if (seconds < 0) {
        snprintf(buffer, 32, "%s --:--:--", prefix);
        return;
    }

    hours = seconds / 3600;
    minutes = (seconds % 3600) / 60;
    remainderSeconds = seconds % 60;
    snprintf(buffer, 32, "%s %02d:%02d:%02d", prefix, hours, minutes, remainderSeconds);
}

static void format_position_text(Position pos, char buffer[8]) {
    if (buffer == NULL) {
        return;
    }

    if (!isValidPosition(pos)) {
        snprintf(buffer, 8, "??");
        return;
    }

    buffer[0] = (char) ('A' + pos.col);
    buffer[1] = (char) ('0' + (8 - pos.row));
    buffer[2] = '\0';
}

static void format_hint_text(Move move, char buffer[64]) {
    char fromText[8];
    char toText[8];

    if (buffer == NULL) {
        return;
    }

    format_position_text(move.from, fromText);
    format_position_text(move.to, toText);
    snprintf(buffer, 64, "Hint: %s -> %s", fromText, toText);
}

static int is_promotion_special(SpecialMove type) {
    return type == PROMOTION_QUEEN
        || type == PROMOTION_ROOK
        || type == PROMOTION_BISHOP
        || type == PROMOTION_KNIGHT;
}

static void gui_set_status_text(Gui *gui, const char *text) {
    if (gui == NULL || !GTK_IS_WIDGET(gui->status_label) || !GTK_IS_LABEL(gui->status_label)) {
        return;
    }

    gtk_label_set_text(GTK_LABEL(gui->status_label), (text != NULL) ? text : "");
}

static void gui_show_message_dialog(Gui *gui, GtkMessageType type,
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

    (void)gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int gui_confirm(Gui *gui, const char *title, const char *message) {
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
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "%s",
        message);
    if (title != NULL) {
        gtk_window_set_title(GTK_WINDOW(dialog), title);
    }

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return response == GTK_RESPONSE_YES;
}

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

static void gui_set_error(Gui *gui, ErrorCode code) {
    const char *message = getErrorMessage(code);

    if (gui != NULL && GTK_IS_WIDGET(gui->status_label) && GTK_IS_LABEL(gui->status_label)) {
        gui_set_status_text(gui, message);
        return;
    }

    gui_show_message_dialog(gui, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Anteater Chess", message);
}

static void gui_rebuild_root_box(Gui *gui, GtkAlign halign, GtkAlign valign, int spacing) {
    if (!gui_window_is_valid(gui)) {
        return;
    }

    if (gui->main_box != NULL && GTK_IS_WIDGET(gui->main_box)) {
        gtk_widget_destroy(gui->main_box);
    }

    gui_clear_view_refs(gui);
    gui->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing);
    gtk_widget_set_halign(gui->main_box, halign);
    gtk_widget_set_valign(gui->main_box, valign);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->main_box);
}

static GtkWidget *create_centered_button(const char *label) {
    GtkWidget *button = gtk_button_new_with_label(label);

    gtk_widget_set_hexpand(button, TRUE);
    gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
    return button;
}

static void build_difficulty_group(GtkWidget *parent, const char *labelText,
                                   GtkWidget *buttons[3]) {
    GtkWidget *label;
    GtkWidget *box;

    label = gtk_label_new(labelText);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);

    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(parent), box, FALSE, FALSE, 0);

    buttons[0] = gtk_radio_button_new_with_label(NULL, "Easy");
    buttons[1] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttons[0]), "Medium");
    buttons[2] = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(buttons[0]), "Hard");
    gtk_box_pack_start(GTK_BOX(box), buttons[0], FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), buttons[1], FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), buttons[2], FALSE, FALSE, 0);
}

static void build_timer_controls(Gui *gui, GtkWidget *parent) {
    GtkWidget *timerBox;
    GtkWidget *hoursLabel;
    GtkWidget *minutesLabel;
    GtkWidget *secondsLabel;

    gui->setup_timer_toggle = gtk_toggle_button_new_with_label("Turn timer");
    gtk_widget_set_halign(gui->setup_timer_toggle, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(parent), gui->setup_timer_toggle, FALSE, FALSE, 0);

    timerBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(timerBox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(parent), timerBox, FALSE, FALSE, 0);

    hoursLabel = gtk_label_new("Hours:");
    gui->setup_hours_spin = gtk_spin_button_new_with_range(0, 1, 1);
    gtk_widget_set_size_request(gui->setup_hours_spin, 60, -1);

    minutesLabel = gtk_label_new("Minutes:");
    gui->setup_minutes_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_widget_set_size_request(gui->setup_minutes_spin, 60, -1);

    secondsLabel = gtk_label_new("Seconds:");
    gui->setup_seconds_spin = gtk_spin_button_new_with_range(0, 59, 1);
    gtk_widget_set_size_request(gui->setup_seconds_spin, 60, -1);

    gui->setup_timer_widgets[0] = hoursLabel;
    gui->setup_timer_widgets[1] = gui->setup_hours_spin;
    gui->setup_timer_widgets[2] = minutesLabel;
    gui->setup_timer_widgets[3] = gui->setup_minutes_spin;
    gui->setup_timer_widgets[4] = secondsLabel;
    gui->setup_timer_widgets[5] = gui->setup_seconds_spin;

    gtk_box_pack_start(GTK_BOX(timerBox), hoursLabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timerBox), gui->setup_hours_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timerBox), minutesLabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timerBox), gui->setup_minutes_spin, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timerBox), secondsLabel, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(timerBox), gui->setup_seconds_spin, FALSE, FALSE, 0);

    g_signal_connect(gui->setup_timer_toggle, "toggled", G_CALLBACK(on_setup_timer_toggled), gui);
}

static void apply_difficulty_selection(GtkWidget *buttons[3], AIDifficulty difficulty) {
    int index = difficulty_index(difficulty);

    if (GTK_IS_TOGGLE_BUTTON(buttons[index])) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[index]), TRUE);
    }
}

static AIDifficulty selected_difficulty(GtkWidget *buttons[3], AIDifficulty fallback) {
    int index;

    for (index = 0; index < 3; ++index) {
        if (GTK_IS_TOGGLE_BUTTON(buttons[index])
            && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttons[index]))) {
            return difficulty_from_index(index);
        }
    }

    return fallback;
}

static void apply_pending_setup_config(Gui *gui) {
    int totalSeconds;
    int hours;
    int minutes;
    int seconds;

    if (gui == NULL) {
        return;
    }

    if (GTK_IS_TOGGLE_BUTTON(gui->setup_timer_toggle)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->setup_timer_toggle),
            gui->pendingConfig.timerEnabled ? TRUE : FALSE);
    }

    totalSeconds = gui->pendingConfig.initialTimeSeconds;
    hours = totalSeconds / 3600;
    minutes = (totalSeconds % 3600) / 60;
    seconds = totalSeconds % 60;
    if (GTK_IS_SPIN_BUTTON(gui->setup_hours_spin)) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui->setup_hours_spin), hours);
    }
    if (GTK_IS_SPIN_BUTTON(gui->setup_minutes_spin)) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui->setup_minutes_spin), minutes);
    }
    if (GTK_IS_SPIN_BUTTON(gui->setup_seconds_spin)) {
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(gui->setup_seconds_spin), seconds);
    }

    if (GTK_IS_TOGGLE_BUTTON(gui->setup_side_white) && GTK_IS_TOGGLE_BUTTON(gui->setup_side_black)) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
            (gui->pendingConfig.playerColor == BLACK) ? gui->setup_side_black : gui->setup_side_white), TRUE);
    }

    if (gui->pendingConfig.mode == MODE_HUMAN_VS_COMPUTER) {
        if (gui->pendingConfig.playerColor == WHITE) {
            apply_difficulty_selection(gui->setup_ai_diff_buttons, gui->pendingConfig.aiDifficultyBlack);
        } else {
            apply_difficulty_selection(gui->setup_ai_diff_buttons, gui->pendingConfig.aiDifficultyWhite);
        }
    } else if (gui->pendingConfig.mode == MODE_COMPUTER_VS_COMPUTER) {
        apply_difficulty_selection(gui->setup_white_diff_buttons, gui->pendingConfig.aiDifficultyWhite);
        apply_difficulty_selection(gui->setup_black_diff_buttons, gui->pendingConfig.aiDifficultyBlack);
    }
}

static int collect_setup_config(Gui *gui, GameConfig *config, ErrorCode *errorCode) {
    int hours;
    int minutes;
    int seconds;
    int totalSeconds;

    if (gui == NULL || config == NULL) {
        if (errorCode != NULL) {
            *errorCode = ERR_FATAL;
        }
        return 1;
    }

    *config = gui->pendingConfig;
    initGameConfigForMode(config, gui->pendingConfig.mode);

    config->timerEnabled = (GTK_IS_TOGGLE_BUTTON(gui->setup_timer_toggle)
        && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->setup_timer_toggle))) ? 1 : 0;

    hours = GTK_IS_SPIN_BUTTON(gui->setup_hours_spin)
        ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui->setup_hours_spin)) : 0;
    minutes = GTK_IS_SPIN_BUTTON(gui->setup_minutes_spin)
        ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui->setup_minutes_spin)) : 0;
    seconds = GTK_IS_SPIN_BUTTON(gui->setup_seconds_spin)
        ? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gui->setup_seconds_spin)) : 0;
    totalSeconds = (hours * 3600) + (minutes * 60) + seconds;

    if (config->timerEnabled) {
        if (totalSeconds <= 0) {
            if (errorCode != NULL) {
                *errorCode = ERR_INVALID_TIMER_SETTING;
            }
            return 1;
        }
        config->initialTimeSeconds = totalSeconds;
    }

    switch (config->mode) {
        case MODE_HUMAN_VS_HUMAN:
            config->playerColor = WHITE;
            config->aiDifficultyWhite = DIFFICULTY_NONE;
            config->aiDifficultyBlack = DIFFICULTY_NONE;
            break;
        case MODE_HUMAN_VS_COMPUTER:
            config->playerColor = (GTK_IS_TOGGLE_BUTTON(gui->setup_side_black)
                && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->setup_side_black)))
                ? BLACK : WHITE;
            if (config->playerColor == WHITE) {
                config->aiDifficultyWhite = DIFFICULTY_NONE;
                config->aiDifficultyBlack = selected_difficulty(gui->setup_ai_diff_buttons, DIFFICULTY_EASY);
            } else {
                config->aiDifficultyWhite = selected_difficulty(gui->setup_ai_diff_buttons, DIFFICULTY_EASY);
                config->aiDifficultyBlack = DIFFICULTY_NONE;
            }
            break;
        case MODE_COMPUTER_VS_COMPUTER:
            config->playerColor = EMPTY_COLOR;
            config->aiDifficultyWhite = selected_difficulty(gui->setup_white_diff_buttons, DIFFICULTY_EASY);
            config->aiDifficultyBlack = selected_difficulty(gui->setup_black_diff_buttons, DIFFICULTY_EASY);
            break;
        default:
            if (errorCode != NULL) {
                *errorCode = ERR_FATAL;
            }
            return 1;
    }

    if (errorCode != NULL) {
        *errorCode = ERR_FATAL;
    }
    return 0;
}

static const char *get_piece_icon(Piece piece) {
    static const char *whiteIcons[] = {
        "gtk-dialog-info",
        "gtk-dialog-warning",
        "gtk-dialog-question",
        "gtk-dialog-error",
        "gtk-dialog-authentication",
        "gtk-dialog-password",
        "gtk-dialog-info"
    };
    static const char *blackIcons[] = {
        "gtk-dialog-warning",
        "gtk-dialog-question",
        "gtk-dialog-error",
        "gtk-dialog-authentication",
        "gtk-dialog-password",
        "gtk-dialog-info",
        "gtk-dialog-warning"
    };
    int index;

    if (!isValidPiece(piece) || piece.type == EMPTY_PIECE) {
        return NULL;
    }

    index = piece.type - ANT;
    if (index < 0 || index >= 7) {
        return NULL;
    }

    return piece.color == WHITE ? whiteIcons[index] : blackIcons[index];
}

static void build_main_menu(Gui *gui) {
    GtkWidget *title;

    gui_rebuild_root_box(gui, GTK_ALIGN_CENTER, GTK_ALIGN_CENTER, 24);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold'>Anteater Chess</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), title, FALSE, FALSE, 0);

    gui->new_game_button = create_centered_button("New Game");
    gui->quit_game_button = create_centered_button("Quit Game");
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->new_game_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->quit_game_button, FALSE, FALSE, 0);

    g_signal_connect(gui->new_game_button, "clicked", G_CALLBACK(on_new_game_clicked), gui);
    g_signal_connect(gui->quit_game_button, "clicked", G_CALLBACK(on_quit_game_clicked), gui);
    gtk_widget_show_all(gui->window);
}

static void build_mode_menu(Gui *gui) {
    GtkWidget *label;
    GtkWidget *button;
    int index;
    static const struct {
        const char *label;
        GameMode mode;
    } modes[] = {
        {"Human vs. Human", MODE_HUMAN_VS_HUMAN},
        {"Human vs. Computer", MODE_HUMAN_VS_COMPUTER},
        {"Computer vs. Computer", MODE_COMPUTER_VS_COMPUTER}
    };

    gui_rebuild_root_box(gui, GTK_ALIGN_CENTER, GTK_ALIGN_CENTER, 24);

    label = gtk_label_new("Game Mode Selection");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), label, FALSE, FALSE, 0);

    for (index = 0; index < 3; ++index) {
        button = create_centered_button(modes[index].label);
        g_object_set_data(G_OBJECT(button), "game-mode", GINT_TO_POINTER(modes[index].mode));
        g_signal_connect(button, "clicked", G_CALLBACK(on_mode_selected), gui);
        gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    }

    button = create_centered_button("Back");
    g_signal_connect(button, "clicked", G_CALLBACK(on_back_clicked), gui);
    gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    gtk_widget_show_all(gui->window);
}

static void build_setup_menu(Gui *gui) {
    GtkWidget *contentBox;
    GtkWidget *label;
    GtkWidget *buttonBox;
    GtkWidget *backButton;
    GtkWidget *startButton;

    gui_rebuild_root_box(gui, GTK_ALIGN_CENTER, GTK_ALIGN_FILL, 24);

    contentBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
    gtk_widget_set_halign(contentBox, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(contentBox, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(gui->main_box), contentBox, TRUE, TRUE, 0);

    label = gtk_label_new(game_mode_title(gui->pendingConfig.mode));
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(contentBox), label, FALSE, FALSE, 0);

    if (gui->pendingConfig.mode == MODE_HUMAN_VS_COMPUTER) {
        GtkWidget *sideLabel = gtk_label_new("Select Side");
        GtkWidget *sideBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

        gtk_widget_set_halign(sideLabel, GTK_ALIGN_CENTER);
        gtk_widget_set_halign(sideBox, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(contentBox), sideLabel, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(contentBox), sideBox, FALSE, FALSE, 0);

        gui->setup_side_white = gtk_radio_button_new_with_label(NULL, "White");
        gui->setup_side_black = gtk_radio_button_new_with_label_from_widget(
            GTK_RADIO_BUTTON(gui->setup_side_white), "Black");
        gtk_box_pack_start(GTK_BOX(sideBox), gui->setup_side_white, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(sideBox), gui->setup_side_black, FALSE, FALSE, 0);

        build_difficulty_group(contentBox, "AI Difficulty", gui->setup_ai_diff_buttons);
    } else if (gui->pendingConfig.mode == MODE_COMPUTER_VS_COMPUTER) {
        build_difficulty_group(contentBox, "White AI Difficulty", gui->setup_white_diff_buttons);
        build_difficulty_group(contentBox, "Black AI Difficulty", gui->setup_black_diff_buttons);
    }

    build_timer_controls(gui, contentBox);

    buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), buttonBox, FALSE, FALSE, 0);

    backButton = gtk_button_new_with_label("Back");
    startButton = gtk_button_new_with_label("Start");
    gtk_box_pack_start(GTK_BOX(buttonBox), backButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), startButton, FALSE, FALSE, 0);

    g_signal_connect(backButton, "clicked", G_CALLBACK(on_back_clicked), gui);
    g_signal_connect(startButton, "clicked", G_CALLBACK(on_start_clicked), gui);

    apply_pending_setup_config(gui);
    gtk_widget_show_all(gui->window);
    on_setup_timer_toggled(GTK_TOGGLE_BUTTON(gui->setup_timer_toggle), gui);
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
    g_signal_connect(submitButton, "clicked", G_CALLBACK(on_submit_move_clicked), gui);

    undoButton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(undoButton),
        gtk_image_new_from_icon_name("gtk-undo", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_size_request(undoButton, 60, 60);
    g_signal_connect(undoButton, "clicked", G_CALLBACK(on_undo_clicked), gui);

    hintButton = gtk_button_new();
    gtk_button_set_image(GTK_BUTTON(hintButton),
        gtk_image_new_from_icon_name("gtk-info", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_size_request(hintButton, 60, 60);
    g_signal_connect(hintButton, "clicked", G_CALLBACK(on_hint_clicked), gui);

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
            const char *icon = get_piece_icon(state->board.cells[row][col]);

            if (icon != NULL) {
                gtk_image_set_from_icon_name(GTK_IMAGE(image), icon, GTK_ICON_SIZE_BUTTON);
            }

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

static void build_gameplay_ui(Gui *gui, const GameState *state) {
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
    g_signal_connect(leaveButton, "clicked", G_CALLBACK(on_leave_game_clicked), gui);

    gtk_widget_show_all(gui->window);
}

static void build_endgame_menu(Gui *gui, const GameState *state) {
    GtkWidget *title;
    GtkWidget *result;
    GtkWidget *newGameButton;
    GtkWidget *mainMenuButton;
    GtkWidget *exitButton;

    gui_rebuild_root_box(gui, GTK_ALIGN_CENTER, GTK_ALIGN_CENTER, 24);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='x-large' weight='bold'>Game Over</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), title, FALSE, FALSE, 0);

    result = gtk_label_new(game_result_text(state->result));
    gtk_widget_set_halign(result, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), result, FALSE, FALSE, 0);

    newGameButton = create_centered_button("New Game");
    mainMenuButton = create_centered_button("Main Menu");
    exitButton = create_centered_button("Exit");
    gtk_box_pack_start(GTK_BOX(gui->main_box), newGameButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), mainMenuButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), exitButton, FALSE, FALSE, 0);

    g_signal_connect(newGameButton, "clicked", G_CALLBACK(on_endgame_new_game_clicked), gui);
    g_signal_connect(mainMenuButton, "clicked", G_CALLBACK(on_endgame_main_menu_clicked), gui);
    g_signal_connect(exitButton, "clicked", G_CALLBACK(on_endgame_exit_clicked), gui);
    gtk_widget_show_all(gui->window);
}

static void build_screen_for_state(Gui *gui, const GameState *state) {
    if (gui == NULL || state == NULL) {
        return;
    }

    switch (state->systemState) {
        case MAIN_MENU_STATE:
            build_main_menu(gui);
            break;
        case GAME_MODE_SELECTION_STATE:
            build_mode_menu(gui);
            break;
        case GAME_SETUP_STATE:
            build_setup_menu(gui);
            break;
        case GAMEPLAY_STATE:
            build_gameplay_ui(gui, state);
            break;
        case END_GAME_MENU_STATE:
            build_endgame_menu(gui, state);
            break;
        case EXIT_STATE:
            gui->should_quit = 1;
            if (gui_window_is_valid(gui)) {
                gtk_widget_destroy(gui->window);
            }
            break;
        case INIT_STATE:
        case GAME_TERMINATION_STATE:
        default:
            break;
    }
}

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

    g_signal_connect(gui->window, "destroy", G_CALLBACK(on_window_destroy), gui);
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
        gui->sync_source_id = g_timeout_add(100, on_sync_tick, gui);
    }

    gtk_widget_show_all(gui->window);
    gtk_main();
}

void gui_sync_from_controller(Gui *gui) {
    const GameState *state;

    if (!gui_window_is_valid(gui)) {
        return;
    }

    if (controllerRunUntilIdle(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    state = controllerGetState(&gui->controller);
    if (state == NULL) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    if (!gui->has_rendered_state || gui->last_rendered_state != state->systemState) {
        build_screen_for_state(gui, state);
        gui->last_rendered_state = state->systemState;
        gui->has_rendered_state = 1;
    }

    if (state->systemState == GAMEPLAY_STATE) {
        update_board(gui, state);
        update_movelist(gui, state);
        update_clock(gui);
        update_timers(gui, state);
        gui_display_turn(gui, state->currentTurn);
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

Controller *gui_get_controller(Gui *gui) {
    if (gui == NULL) {
        return NULL;
    }

    return &gui->controller;
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

void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf) {
    if (gui == NULL || row < 0 || row >= 8 || col < 0 || col >= 10) {
        return;
    }

    if (!GTK_IS_IMAGE(gui->board_images[row][col])) {
        return;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(gui->board_images[row][col]), pixbuf);
}

void update_board(Gui *gui, const GameState *state) {
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

            icon = get_piece_icon(state->board.cells[row][col]);
            if (icon != NULL) {
                gtk_image_set_from_icon_name(GTK_IMAGE(gui->board_images[row][col]),
                    icon,
                    GTK_ICON_SIZE_BUTTON);
            } else {
                gtk_image_clear(GTK_IMAGE(gui->board_images[row][col]));
            }
        }
    }
}

void update_movelist(Gui *gui, const GameState *state) {
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

        format_position_text(move.from, fromText);
        format_position_text(move.to, toText);
        g_string_append_printf(text, "%d. %s-%s\n", index + 1, fromText, toText);
    }

    gtk_text_buffer_set_text(buffer, text->str, -1);
    g_string_free(text, TRUE);
}

void update_clock(Gui *gui) {
    char timeText[32];

    if (gui == NULL || !GTK_IS_WIDGET(gui->time_display) || !GTK_IS_LABEL(gui->time_display)) {
        return;
    }

    format_elapsed_text(timeText, getElapsedTimeSeconds());
    gtk_label_set_text(GTK_LABEL(gui->time_display), timeText);
}

void update_timers(Gui *gui, const GameState *state) {
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
    format_timer_text(whiteText, "White", whiteRemaining);
    format_timer_text(blackText, "Black", blackRemaining);
    gtk_label_set_text(GTK_LABEL(gui->white_timer_label), whiteText);
    gtk_label_set_text(GTK_LABEL(gui->black_timer_label), blackText);
}

void gui_display_turn(Gui *gui, Color turn) {
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

static gboolean on_sync_tick(gpointer user_data) {
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

static void on_window_destroy(GtkWidget *widget, gpointer user_data) {
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

static void on_new_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

static void on_quit_game_clicked(GtkButton *button, gpointer user_data) {
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

static void on_mode_selected(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    GameMode mode = (GameMode) GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "game-mode"));

    initGameConfigForMode(&gui->pendingConfig, mode);
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

static void on_back_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestBack(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

static void on_start_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    GameConfig config;
    ErrorCode errorCode;

    (void)button;
    if (collect_setup_config(gui, &config, &errorCode) != 0) {
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

static void on_setup_timer_toggled(GtkToggleButton *button, gpointer user_data) {
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

static void on_submit_move_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    const GameState *state;
    const char *fromText;
    const char *toText;
    Command command;
    MoveRequest request;
    Move resolvedMove;

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

    if (current_turn_is_ai(state)) {
        gui_set_error(gui, ERR_NOT_YOUR_TURN);
        return;
    }

    fromText = gtk_entry_get_text(GTK_ENTRY(gui->from_entry));
    toText = gtk_entry_get_text(GTK_ENTRY(gui->to_entry));
    if (parseMoveCommand(fromText, toText, &command) != 0) {
        gui_set_error(gui, ERR_INVALID_MOVE_FORMAT);
        return;
    }

    if (createMoveRequestFromCommand(&request, command) != 0
        || resolveMoveRequest(state, request, &resolvedMove) != 0) {
        gui_set_error(gui, ERR_ILLEGAL_MOVE);
        return;
    }

    if (is_promotion_special(resolvedMove.specialType)) {
        PromotionChoice promotion;

        if (gui_select_promotion_choice(gui, &promotion) != 0) {
            return;
        }
        request.promotion = promotion;
    }

    if (controllerSubmitMoveRequest(&gui->controller, request) != 0) {
        gui_set_error(gui, ERR_ILLEGAL_MOVE);
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(gui->from_entry), "");
    gtk_entry_set_text(GTK_ENTRY(gui->to_entry), "");
    gui_set_status_text(gui, "");
    gui_sync_from_controller(gui);
}

static void on_undo_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestUndo(&gui->controller) != 0) {
        gui_set_error(gui, ERR_UNDO_UNAVAILABLE);
        return;
    }

    gui_set_status_text(gui, "");
    gui_sync_from_controller(gui);
}

static void on_hint_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    Move move;
    char hintText[64];

    (void)button;
    if (controllerGetHint(&gui->controller, &move) != 0) {
        gui_set_error(gui, ERR_HINT_UNAVAILABLE);
        return;
    }

    format_hint_text(move, hintText);
    gui_set_status_text(gui, hintText);
}

static void on_leave_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestLeaveGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

static void on_endgame_new_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

static void on_endgame_main_menu_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestBack(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

static void on_endgame_exit_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (controllerRequestExit(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}
