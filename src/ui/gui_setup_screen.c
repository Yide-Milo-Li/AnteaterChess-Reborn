#include "gui_internal.h"

#include <stdio.h>

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

    g_signal_connect(gui->setup_timer_toggle, "toggled", G_CALLBACK(gui_on_setup_timer_toggled), gui);
}

static void apply_difficulty_selection(GtkWidget *buttons[3], AIDifficulty difficulty) {
    int index = gui_difficulty_index(difficulty);

    if (GTK_IS_TOGGLE_BUTTON(buttons[index])) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(buttons[index]), TRUE);
    }
}

static AIDifficulty selected_difficulty(GtkWidget *buttons[3], AIDifficulty fallback) {
    int index;

    for (index = 0; index < 3; ++index) {
        if (GTK_IS_TOGGLE_BUTTON(buttons[index])
            && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(buttons[index]))) {
            return gui_difficulty_from_index(index);
        }
    }

    return fallback;
}

static void build_ai_budget_summary(GtkWidget *parent) {
    GtkWidget *label;
    char text[128];

    snprintf(text, sizeof(text), "AI Budget: Easy %dms / Medium %dms / Hard %dms",
        getDefaultAITimeBudgetMs(DIFFICULTY_EASY),
        getDefaultAITimeBudgetMs(DIFFICULTY_MEDIUM),
        getDefaultAITimeBudgetMs(DIFFICULTY_HARD));
    label = gtk_label_new(text);
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0);
}

void gui_apply_pending_setup_config(Gui *gui) {
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

int gui_collect_setup_config(Gui *gui, GameConfig *config, ErrorCode *errorCode) {
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
    config->aiTimeLimit = 0;

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

void gui_build_setup_menu(Gui *gui) {
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

    label = gtk_label_new(gui_game_mode_title(gui->pendingConfig.mode));
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
        build_ai_budget_summary(contentBox);
    } else if (gui->pendingConfig.mode == MODE_COMPUTER_VS_COMPUTER) {
        build_difficulty_group(contentBox, "White AI Difficulty", gui->setup_white_diff_buttons);
        build_difficulty_group(contentBox, "Black AI Difficulty", gui->setup_black_diff_buttons);
        build_ai_budget_summary(contentBox);
    }

    build_timer_controls(gui, contentBox);

    buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), buttonBox, FALSE, FALSE, 0);

    backButton = gtk_button_new_with_label("Back");
    startButton = gtk_button_new_with_label("Start");
    gtk_box_pack_start(GTK_BOX(buttonBox), backButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), startButton, FALSE, FALSE, 0);

    g_signal_connect(backButton, "clicked", G_CALLBACK(gui_on_back_clicked), gui);
    g_signal_connect(startButton, "clicked", G_CALLBACK(gui_on_start_clicked), gui);

    gui_apply_pending_setup_config(gui);
    gtk_widget_show_all(gui->window);
    gui_on_setup_timer_toggled(GTK_TOGGLE_BUTTON(gui->setup_timer_toggle), gui);
}
