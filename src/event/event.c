#include "system/event.h"

#include <string.h>

/*
 * Alignment assumptions for future extensions:
 * - event.h is the truth source for every public constructor in this file.
 * - Event creation stays payload-focused and must not perform queue routing or FSM work.
 * - Payload-free system events should be representable without touching unrelated union fields.
 */

/* Zero-initialize one event so unused union bytes stay deterministic. */
static Event blankEvent(EventType type) {
    Event event;

    memset(&event, 0, sizeof(event));
    event.type = type;
    return event;
}

/* Build one human-selected move event after controller-side request resolution. */
Event createPlayerMoveEvent(Move move) {
    Event event = blankEvent(EVENT_PLAYER_MOVE);

    event.data.move = move;
    return event;
}

/* Build one AI move event from an already selected move payload. */
Event createAIMoveEvent(Move move) {
    Event event = blankEvent(EVENT_AI_MOVE);

    event.data.move = move;
    return event;
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
    Event event = blankEvent(EVENT_ERROR);

    event.data.errorCode = code;
    return event;
}

/* Build a fatal error event carrying one error code. */
Event createFatalErrorEvent(ErrorCode code) {
    Event event = blankEvent(EVENT_FATAL_ERROR);

    event.data.errorCode = code;
    return event;
}
