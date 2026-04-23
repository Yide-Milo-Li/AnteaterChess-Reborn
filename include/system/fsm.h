#ifndef CHESS_SYSTEM_FSM_H
#define CHESS_SYSTEM_FSM_H

#include "core/gamestate.h"
#include "system/event.h"
#include "system/system_state.h"

/*
 * Compatibility surface:
 * - These entrypoints remain exported for existing tests and temporary
 *   frontends.
 * - Future UI layers should prefer integrating through controller.h instead of
 *   depending on the FSM directly.
 * - FSM remains the internal event/state execution engine behind that public
 *   controller layer.
 */
int processEvent(GameState *state, Event event);
int transitionState(GameState *state, SystemState newState);

#endif
