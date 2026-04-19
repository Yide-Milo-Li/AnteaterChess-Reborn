#include <gtk/gtk.h>
#include "ui/gui.h"
#include "ui/dialog.h"



static void setup_quit_confirmation(Gui *gui);
static void setup_main_menu(Gui *gui);

static void on_new_game_clicked(GtkButton *button, gpointer user_data) {
    (void)button;
    (void)user_data;
    g_print("New Game button clicked\n");
}

static void on_quit_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *)user_data;
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

static void setup_main_menu(Gui *gui) {
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
    g_signal_connect(gui->new_game_button, "clicked", G_CALLBACK(on_new_game_clicked), gui);
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
    g_signal_connect(gui->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    setup_main_menu(gui);

    return gui;
}

void gui_destroy(Gui *gui) {
    if (gui == NULL) {
        return;
    }
    if (gui->window != NULL && GTK_IS_WIDGET(gui->window)) {
        gtk_widget_destroy(gui->window);
    }
    g_free(gui);
}

void gui_run(Gui *gui) {
    (void)gui;
    gtk_main();
}

void gui_update(Gui *gui) {
    if (gui == NULL || gui->window == NULL) {
        return;
    }
    gtk_widget_queue_draw(gui->window);
}
