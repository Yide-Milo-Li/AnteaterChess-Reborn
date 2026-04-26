
#ifndef CHESS_UI_GUI_H
#define CHESS_UI_GUI_H

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"

typedef struct Gui Gui;

#define GUI_MOVE_PROVIDER_OK 0
#define GUI_MOVE_PROVIDER_UNAVAILABLE 1
#define GUI_MOVE_PROVIDER_TIME_FORFEIT 2

typedef int (*GuiMoveProvider)(const GameState *state, Move *move, void *context);
typedef int (*GuiHintProvider)(const GameState *state, Move *move, void *context);
typedef void (*GuiMoveProviderReset)(void *context, const GameConfig *config);

Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);
void gui_set_move_provider(Gui *gui, GuiMoveProvider provider, void *context);
void gui_set_move_provider_reset(Gui *gui,
                                 GuiMoveProviderReset reset,
                                 void *context);
void gui_set_hint_provider(Gui *gui, GuiHintProvider provider, void *context);

#endif
