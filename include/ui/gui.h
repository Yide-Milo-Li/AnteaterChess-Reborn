
#ifndef CHESS_UI_GUI_H
#define CHESS_UI_GUI_H
/*
* Declaration of the main GUI struct and functions for creating, destroying, and running the GUI, as well as setting up the menus, and processing events. This file also includes getters and setters for menu selections.
* The functions in this file should not be needed outside of the UI module
* All external interaction with the GUI should be done through the board renderer interface (e.g., renderBoard, br_process_events) and the menu selection getters (e.g., getMainMenuSelection, getGameModeSelection).
*/
#include <gtk/gtk.h>

#include "dialog.h"
#include "endgame_menu.h"
#include "error_popup.h"
#include "fatal_error_window.h"
#include "game_mode_menu.h"
#include "game_setup_menu.h"
#include "gameplay_ui.h"
#include "main_menu.h"
#include "move_input_widget.h"
#include "core/gamestate.h"

typedef struct Gui Gui;
// Set the active GameState for UI callbacks
void set_ui_active_gamestate(GameState *state);
Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);

typedef struct Gui {
	GtkWidget *window;
	GtkWidget *main_box;
	GtkWidget *new_game_button;
	GtkWidget *quit_game_button;
	GtkWidget *board_images[8][10];
} Gui;

void setup_main_menu(Gui *gui);
void setup_game_mode_menu(Gui *gui);
void setup_game_setup_menu_human_vs_human(Gui *gui);
void setup_game_setup_menu_human_vs_computer(Gui *gui);
void setup_game_setup_menu_computer_vs_computer(Gui *gui);
void setup_gameplay_ui(Gui *gui, const GameState *gameState);
void gui_reset_selections(void);
void gui_process_events(void);
int gui_window_is_valid(Gui *gui);
void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf);

#endif // CHESS_UI_GUI_H
