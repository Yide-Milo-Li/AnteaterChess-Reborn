#ifndef CHESS_GAMEPLAY_MOVE_RESOLVER_H
#define CHESS_GAMEPLAY_MOVE_RESOLVER_H

#include "core/gamestate.h"
#include "core/move.h"
#include "input/move_request.h"

int resolveMoveRequest(const GameState *state, MoveRequest request,
                       Move *resolvedMove);

#endif
