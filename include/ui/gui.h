
#ifndef CHESS_UI_GUI_H
#define CHESS_UI_GUI_H

#include <gtk/gtk.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "system/controller.h"

typedef struct Gui Gui;

Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);

void gui_sync_from_controller(Gui *gui);
int gui_render_snapshot(Gui *gui, const GameState *state);
const GameState *gui_get_state(const Gui *gui);
Controller *gui_get_controller(Gui *gui);

int gui_process_events(void);
int gui_window_is_valid(const Gui *gui);
void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf);

void update_board(Gui *gui, const GameState *state);
void update_movelist(Gui *gui, const GameState *state);
void update_clock(Gui *gui);
void update_timers(Gui *gui, const GameState *state);
void gui_display_turn(Gui *gui, Color turn);

#endif
