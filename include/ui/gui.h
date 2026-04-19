#ifndef CHESS_UI_GUI_H
#define CHESS_UI_GUI_H

#include <gtk/gtk.h>

#include "board_renderer.h"
#include "dialog.h"
#include "endgame_menu.h"
#include "error_popup.h"
#include "fatal_error_window.h"
#include "game_mode_menu.h"
#include "game_setup_menu.h"
#include "gameplay_ui.h"
#include "main_menu.h"
#include "move_input_widget.h"

typedef struct Gui Gui;

Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);

typedef struct Gui {
	GtkWidget *window;
	GtkWidget *main_box;
	GtkWidget *new_game_button;
	GtkWidget *quit_game_button;
} Gui;

#endif // CHESS_UI_GUI_H
