#include "system/event.h"

#include <string.h>

/* Zero-initialize makes test output / memcmp-based assertions stable. */
static Event blankEvent(EventType type) {
    Event e;
    memset(&e, 0, sizeof e);
    e.type = type;
    return e;
}

Event createMoveInputEvent(Command cmd) {
    Event e = blankEvent(EVENT_MOVE_INPUT);
    e.data.command = cmd;
    return e;
}

Event createAIMoveEvent(Move move) {
    Event e = blankEvent(EVENT_AI_MOVE);
    e.data.move = move;
    return e;
}

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

Event createUndoEvent(void) {
    return blankEvent(EVENT_UNDO);
}

Event createErrorEvent(ErrorCode code) {
    Event e = blankEvent(EVENT_ERROR);
    e.data.errorCode = code;
    return e;
}

Event createFatalErrorEvent(ErrorCode code) {
    Event e = blankEvent(EVENT_FATAL_ERROR);
    e.data.errorCode = code;
    return e;
}

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