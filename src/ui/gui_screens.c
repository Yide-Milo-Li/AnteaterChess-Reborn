#include "gui_internal.h"

#include "time/clock.h"

static GtkWidget *gui_create_menu_panel(void) {
    GtkWidget *panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

    gtk_widget_set_halign(panel, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(panel, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(panel, 360, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(panel), "menu-panel");
    return panel;
}

void gui_build_main_menu(Gui *gui) {
    GtkWidget *panel;
    GtkWidget *title;

    gui_rebuild_root_box(gui, GTK_ALIGN_FILL, GTK_ALIGN_FILL, 0);
    panel = gui_create_menu_panel();
    gtk_box_pack_start(GTK_BOX(gui->main_box), panel, TRUE, FALSE, 0);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold'>Anteater Chess</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), title, FALSE, FALSE, 0);

    gui->new_game_button = gui_create_centered_button("New Game");
    gui->quit_game_button = gui_create_centered_button("Quit Game");
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->quit_game_button), "destructive-button");
    gtk_box_pack_start(GTK_BOX(panel), gui->new_game_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), gui->quit_game_button, FALSE, FALSE, 0);

    g_signal_connect(gui->new_game_button, "clicked", G_CALLBACK(gui_on_new_game_clicked), gui);
    g_signal_connect(gui->quit_game_button, "clicked", G_CALLBACK(gui_on_quit_game_clicked), gui);
    gtk_widget_show_all(gui->window);
}

void gui_build_mode_menu(Gui *gui) {
    GtkWidget *panel;
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

    gui_rebuild_root_box(gui, GTK_ALIGN_FILL, GTK_ALIGN_FILL, 0);
    panel = gui_create_menu_panel();
    gtk_widget_set_size_request(panel, 420, -1);
    gtk_box_pack_start(GTK_BOX(gui->main_box), panel, TRUE, FALSE, 0);

    label = gtk_label_new("Game Mode Selection");
    gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
    gtk_style_context_add_class(gtk_widget_get_style_context(label), "panel-title");
    gtk_box_pack_start(GTK_BOX(panel), label, FALSE, FALSE, 0);

    for (index = 0; index < (int)(sizeof(modes) / sizeof(modes[0])); ++index) {
        button = gui_create_centered_button(modes[index].label);
        g_object_set_data(G_OBJECT(button), "game-mode", GINT_TO_POINTER(modes[index].mode));
        g_signal_connect(button, "clicked", G_CALLBACK(gui_on_mode_selected), gui);
        gtk_box_pack_start(GTK_BOX(panel), button, FALSE, FALSE, 0);
    }

    button = gui_create_centered_button("Back");
    g_signal_connect(button, "clicked", G_CALLBACK(gui_on_back_clicked), gui);
    gtk_box_pack_start(GTK_BOX(panel), button, FALSE, FALSE, 0);
    gtk_widget_show_all(gui->window);
}

static void gui_on_endgame_dialog_destroy(GtkWidget *widget, gpointer user_data) {
    Gui *gui = (Gui *)user_data;

    if (gui != NULL && gui->endgame_dialog == widget) {
        gui->endgame_dialog = NULL;
    }
}

void gui_build_endgame_menu(Gui *gui, const GameState *state) {
    GtkWidget *dialog;
    GtkWidget *panel;
    GtkWidget *title;
    GtkWidget *result;
    GtkWidget *clock;
    GtkWidget *newGameButton;
    GtkWidget *mainMenuButton;
    GtkWidget *exitButton;
    char timeText[32];

    if (gui == NULL || state == NULL || !gui_window_is_valid(gui)) {
        return;
    }
    if (GTK_IS_WIDGET(gui->endgame_dialog)) {
        gtk_window_present(GTK_WINDOW(gui->endgame_dialog));
        return;
    }

    dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gui->endgame_dialog = dialog;
    gtk_window_set_title(GTK_WINDOW(dialog), "Game Over");
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui->window));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_modal(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_deletable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_container_set_border_width(GTK_CONTAINER(dialog), 18);
    g_signal_connect(dialog, "destroy", G_CALLBACK(gui_on_endgame_dialog_destroy), gui);

    panel = gui_create_menu_panel();
    gtk_container_add(GTK_CONTAINER(dialog), panel);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='x-large' weight='bold'>Game Over</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), title, FALSE, FALSE, 0);

    result = gtk_label_new(gui_game_result_text(state->result));
    gtk_widget_set_halign(result, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), result, FALSE, FALSE, 0);

    gui_format_elapsed_text(timeText, getElapsedTimeSeconds());
    clock = gtk_label_new(timeText);
    gtk_widget_set_halign(clock, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(panel), clock, FALSE, FALSE, 0);

    newGameButton = gui_create_centered_button("New Game");
    mainMenuButton = gui_create_centered_button("Main Menu");
    exitButton = gui_create_centered_button("Exit");
    gui_set_button_icon(exitButton, "alert-triangle-svgrepo-com.svg", 18);
    gtk_style_context_add_class(gtk_widget_get_style_context(exitButton), "destructive-button");
    gtk_box_pack_start(GTK_BOX(panel), newGameButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), mainMenuButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(panel), exitButton, FALSE, FALSE, 0);

    g_signal_connect(newGameButton, "clicked", G_CALLBACK(gui_on_endgame_new_game_clicked), gui);
    g_signal_connect(mainMenuButton, "clicked", G_CALLBACK(gui_on_endgame_main_menu_clicked), gui);
    g_signal_connect(exitButton, "clicked", G_CALLBACK(gui_on_endgame_exit_clicked), gui);
    gtk_widget_show_all(dialog);
    gtk_window_present(GTK_WINDOW(dialog));
}

void gui_build_screen_for_state(Gui *gui, const GameState *state) {
    if (gui == NULL || state == NULL) {
        return;
    }

    switch (state->systemState) {
        case MAIN_MENU_STATE:
            gui_destroy_endgame_dialog(gui);
            gui_build_main_menu(gui);
            break;
        case GAME_MODE_SELECTION_STATE:
            gui_destroy_endgame_dialog(gui);
            gui_build_mode_menu(gui);
            break;
        case GAME_SETUP_STATE:
            gui_destroy_endgame_dialog(gui);
            gui_build_setup_menu(gui);
            break;
        case GAMEPLAY_STATE:
            gui_destroy_endgame_dialog(gui);
            gui_build_gameplay_ui(gui, state);
            break;
        case END_GAME_MENU_STATE:
            gui_build_gameplay_ui(gui, state);
            gui_build_endgame_menu(gui, state);
            break;
        case EXIT_STATE:
            gui_destroy_endgame_dialog(gui);
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
