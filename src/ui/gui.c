

#include <gtk/gtk.h>
#include "ui/game_setup_menu.h"
#include "ui/gui.h"


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

static void on_game_mode_back_clicked(GtkButton *button, gpointer user_data) {
    setBackButtonClicked(1);
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
            g_signal_connect(button, "clicked", G_CALLBACK(on_game_mode_back_clicked), NULL);
        }
        gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(gui->window);
}

void gui_reset_selections(void) {
    resetMainMenuSelection();
    resetGameModeSelection();
    resetBackButtonClicked();
    resetStartPressed();
}

void gui_process_events(void) {
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
}

int gui_window_is_valid(Gui *gui) {
    return gui && GTK_IS_WIDGET(gui->window);
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
    int active = gtk_toggle_button_get_active(button);
    setTimerEnabled(active);
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
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_timer_toggled_config), NULL);
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_timer_toggled_config), NULL);

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
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_timer_toggled_config), NULL);
    g_signal_connect(timer_toggle, "toggled", G_CALLBACK(on_timer_toggled_config), NULL);

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