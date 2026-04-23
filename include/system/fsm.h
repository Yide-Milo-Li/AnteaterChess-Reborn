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
 * - FSM remains the event/state execution engine behind controller.h. It owns
 *   event meaning, transition legality, and gameplay side effects.
 * - processEvent() accepts EVENT_NONE for lifecycle compatibility. New callers
 *   should let Controller synthesize that handshake instead.
 */
int processEvent(GameState *state, Event event);
int transitionState(GameState *state, SystemState newState);

#endif
