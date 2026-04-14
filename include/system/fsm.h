#ifndef CHESS_SYSTEM_FSM_H
#define CHESS_SYSTEM_FSM_H

#include "core/gamestate.h"
#include "system/event.h"
#include "system/system_state.h"

int processEvent(GameState *state, Event event);
int transitionState(GameState *state, SystemState newState);

#endif
