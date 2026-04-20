#ifndef CHESS_UI_BOARD_RENDERER_H
#define CHESS_UI_BOARD_RENDERER_H

#include "core/gamestate.h"

// Forward declaration for Gui
struct Gui;
typedef struct Gui Gui;

// Getter for the static gui pointer
Gui *get_board_renderer_gui(void);

// Event processing and validation functions
void br_process_events(void);
int br_window_is_valid(Gui *gui);

int renderBoard(const GameState *state);

#endif // CHESS_UI_BOARD_RENDERER_H
