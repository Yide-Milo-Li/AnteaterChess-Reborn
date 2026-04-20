#include "system/event.h"

#include <string.h>

/*
 * Alignment assumptions for future extensions:
 * - event.h is the truth source for every public constructor in this file.
 * - Event creation stays payload-focused and must not perform queue routing or FSM work.
 * - Payload-free system events should be representable without touching unrelated union fields.
 */

/* Zero-initialize makes test output / memcmp-based assertions stable. */
static Event blankEvent(EventType type) {
    Event e;
    memset(&e, 0, sizeof e);
    e.type = type;
    return e;
}

/* Build one gameplay input event from a parsed command payload. */
Event createMoveInputEvent(Command cmd) {
    Event e = blankEvent(EVENT_MOVE_INPUT);
    e.data.command = cmd;
    return e;
}

/* Build one AI move event from an already selected move payload. */
Event createAIMoveEvent(Move move) {
    Event e = blankEvent(EVENT_AI_MOVE);
    e.data.move = move;
    return e;
}

/* Build one payload-free system event when the type does not need union data. */
Event createSystemEvent(EventType type) {
    switch (type) {
        case EVENT_UNDO:
        case EVENT_TIMER_EXPIRED:
        case EVENT_LEAVE_GAME:
        case EVENT_BACK:
        case EVENT_NEW_GAME:
        case EVENT_EXIT_PROGRAM:
        case EVENT_HINT:
        case EVENT_NONE:
            return blankEvent(type);
        default:
            return blankEvent(EVENT_NONE);
    }
}

/* Build the dedicated undo event helper used across controller and tests. */
Event createUndoEvent(void) {
    return blankEvent(EVENT_UNDO);
}

/* Build a recoverable error event carrying one error code. */
Event createErrorEvent(ErrorCode code) {
    Event e = blankEvent(EVENT_ERROR);
    e.data.errorCode = code;
    return e;
}

/* Build a fatal error event carrying one error code. */
Event createFatalErrorEvent(ErrorCode code) {
    Event e = blankEvent(EVENT_FATAL_ERROR);
    e.data.errorCode = code;
    return e;
}

/* Convert an event type into a stable debug label for diagnostics. */
const char *eventTypeName(EventType t) {
    switch (t) {
        case EVENT_MOVE_INPUT:    return "EVENT_MOVE_INPUT";
        case EVENT_AI_MOVE:       return "EVENT_AI_MOVE";
        case EVENT_UNDO:          return "EVENT_UNDO";
        case EVENT_TIMER_EXPIRED: return "EVENT_TIMER_EXPIRED";
        case EVENT_LEAVE_GAME:    return "EVENT_LEAVE_GAME";
        case EVENT_BACK:          return "EVENT_BACK";
        case EVENT_NEW_GAME:      return "EVENT_NEW_GAME";
        case EVENT_EXIT_PROGRAM:  return "EVENT_EXIT_PROGRAM";
        case EVENT_ERROR:         return "EVENT_ERROR";
        case EVENT_FATAL_ERROR:   return "EVENT_FATAL_ERROR";
        case EVENT_HINT:          return "EVENT_HINT";
        case EVENT_NONE:          return "EVENT_NONE";
        default:                  return "UNKNOWN";
    }
}
