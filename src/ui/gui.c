

#include <gtk/gtk.h>
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
static void on_game_mode_button_clicked(GtkButton *button, gpointer user_data) {
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
        "Human vs. Computer",
        "Human vs. Human",
        "Computer vs. Computer",
        "Back"
    };
    for (int i = 0; i < 4; ++i) {
        GtkWidget *button = gtk_button_new_with_label(button_labels[i]);
        gtk_widget_set_hexpand(button, TRUE);
        gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
        g_signal_connect(button, "clicked", G_CALLBACK(on_game_mode_button_clicked), GINT_TO_POINTER(i));
        gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(gui->window);
}

void gui_reset_selections(void) {
    setMainMenuSelection(-1);
    setGameModeSelection(-1);
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
}

static void on_start_clicked(GtkButton *button, gpointer user_data) {
    g_print("Start button clicked\n");
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