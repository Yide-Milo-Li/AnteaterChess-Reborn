#include "gui_internal.h"

#include "time/clock.h"

static void gui_show_endgame_dialog(Gui *gui, const GameState *state) {
    GtkWidget *dialog;
    GtkWidget *content;
    GtkWidget *result;
    GtkWidget *clock;
    char timeText[32];

    if (!gui_window_is_valid(gui) || state == NULL || gui->endgame_dialog_shown) {
        return;
    }

    dialog = gtk_dialog_new_with_buttons("Game Over",
        GTK_WINDOW(gui->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "New Game",
        GTK_RESPONSE_ACCEPT,
        "Main Menu",
        GTK_RESPONSE_APPLY,
        "Exit",
        GTK_RESPONSE_CLOSE,
        NULL);

    content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    result = gtk_label_new(gui_game_result_text(state->result));
    gui_format_elapsed_text(timeText, getElapsedTimeSeconds());
    clock = gtk_label_new(timeText);
    gtk_box_pack_start(GTK_BOX(content), result, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(content), clock, FALSE, FALSE, 8);

    gui->endgame_dialog_shown = 1;
    g_signal_connect(dialog, "response", G_CALLBACK(gui_on_endgame_dialog_response), gui);
    gtk_widget_show_all(dialog);
}

void gui_build_main_menu(Gui *gui) {
    GtkWidget *title;

    gui_rebuild_root_box(gui, GTK_ALIGN_CENTER, GTK_ALIGN_CENTER, 24);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='xx-large' weight='bold'>Anteater Chess</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), title, FALSE, FALSE, 0);

    gui->new_game_button = gui_create_centered_button("New Game");
    gui->quit_game_button = gui_create_centered_button("Quit Game");
    gui_set_button_icon(gui->quit_game_button, "alert-triangle-svgrepo-com.svg", 18);
    gtk_style_context_add_class(gtk_widget_get_style_context(gui->quit_game_button), "destructive-button");
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->new_game_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), gui->quit_game_button, FALSE, FALSE, 0);

    g_signal_connect(gui->new_game_button, "clicked", G_CALLBACK(gui_on_new_game_clicked), gui);
    g_signal_connect(gui->quit_game_button, "clicked", G_CALLBACK(gui_on_quit_game_clicked), gui);
    gtk_widget_show_all(gui->window);
}

void gui_build_mode_menu(Gui *gui) {
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

    for (index = 0; index < (int)(sizeof(modes) / sizeof(modes[0])); ++index) {
        button = gui_create_centered_button(modes[index].label);
        g_object_set_data(G_OBJECT(button), "game-mode", GINT_TO_POINTER(modes[index].mode));
        g_signal_connect(button, "clicked", G_CALLBACK(gui_on_mode_selected), gui);
        gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    }

    button = gui_create_centered_button("Back");
    g_signal_connect(button, "clicked", G_CALLBACK(gui_on_back_clicked), gui);
    gtk_box_pack_start(GTK_BOX(gui->main_box), button, FALSE, FALSE, 0);
    gtk_widget_show_all(gui->window);
}

void gui_build_endgame_menu(Gui *gui, const GameState *state) {
    GtkWidget *title;
    GtkWidget *result;
    GtkWidget *clock;
    GtkWidget *newGameButton;
    GtkWidget *mainMenuButton;
    GtkWidget *exitButton;
    char timeText[32];

    gui_rebuild_root_box(gui, GTK_ALIGN_CENTER, GTK_ALIGN_CENTER, 24);

    title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span size='x-large' weight='bold'>Game Over</span>");
    gtk_widget_set_halign(title, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), title, FALSE, FALSE, 0);

    result = gtk_label_new(gui_game_result_text(state->result));
    gtk_widget_set_halign(result, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), result, FALSE, FALSE, 0);

    gui_format_elapsed_text(timeText, getElapsedTimeSeconds());
    clock = gtk_label_new(timeText);
    gtk_widget_set_halign(clock, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(gui->main_box), clock, FALSE, FALSE, 0);

    newGameButton = gui_create_centered_button("New Game");
    mainMenuButton = gui_create_centered_button("Main Menu");
    exitButton = gui_create_centered_button("Exit");
    gui_set_button_icon(exitButton, "alert-triangle-svgrepo-com.svg", 18);
    gtk_style_context_add_class(gtk_widget_get_style_context(exitButton), "destructive-button");
    gtk_box_pack_start(GTK_BOX(gui->main_box), newGameButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), mainMenuButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(gui->main_box), exitButton, FALSE, FALSE, 0);

    g_signal_connect(newGameButton, "clicked", G_CALLBACK(gui_on_endgame_new_game_clicked), gui);
    g_signal_connect(mainMenuButton, "clicked", G_CALLBACK(gui_on_endgame_main_menu_clicked), gui);
    g_signal_connect(exitButton, "clicked", G_CALLBACK(gui_on_endgame_exit_clicked), gui);
    gtk_widget_show_all(gui->window);
}

void gui_build_screen_for_state(Gui *gui, const GameState *state) {
    if (gui == NULL || state == NULL) {
        return;
    }

    switch (state->systemState) {
        case MAIN_MENU_STATE:
            gui_build_main_menu(gui);
            break;
        case GAME_MODE_SELECTION_STATE:
            gui_build_mode_menu(gui);
            break;
        case GAME_SETUP_STATE:
            gui_build_setup_menu(gui);
            break;
        case GAMEPLAY_STATE:
            gui->endgame_dialog_shown = 0;
            gui_build_gameplay_ui(gui, state);
            break;
        case END_GAME_MENU_STATE:
            if (gui->last_rendered_state == GAMEPLAY_STATE && GTK_IS_WIDGET(gui->main_box)) {
                gui_show_endgame_dialog(gui, state);
            } else {
                gui_build_endgame_menu(gui, state);
            }
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
