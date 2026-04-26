
#ifndef CHESS_UI_GUI_H
#define CHESS_UI_GUI_H

#include "core/gamestate.h"
#include "core/move.h"

typedef struct Gui Gui;

typedef int (*GuiMoveProvider)(const GameState *state, Move *move, void *context);
typedef int (*GuiHintProvider)(const GameState *state, Move *move, void *context);

Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);
void gui_set_move_provider(Gui *gui, GuiMoveProvider provider, void *context);
void gui_set_hint_provider(Gui *gui, GuiHintProvider provider, void *context);

#endif
