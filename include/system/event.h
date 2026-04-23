#ifndef CHESS_SYSTEM_EVENT_H
#define CHESS_SYSTEM_EVENT_H

#include "core/move.h"
#include "error/error_code.h"
#include "input/command.h"

typedef enum {
    /* Legacy command payload. Controller resolves it before FSM dispatch. */
    EVENT_MOVE_INPUT,
    EVENT_PLAYER_MOVE,
    EVENT_AI_MOVE,
    EVENT_UNDO,
    EVENT_TIMER_EXPIRED,
    EVENT_LEAVE_GAME,
    EVENT_BACK,
    EVENT_NEW_GAME,
    EVENT_EXIT_PROGRAM,
    EVENT_ERROR,
    EVENT_FATAL_ERROR,
    /* Compatibility-only as an event. New callers should request hints through
     * controllerGetHint(), which is a read-only query rather than an FSM event. */
    EVENT_HINT,
    /* Compatibility sentinel used by queues and controller-owned lifecycle
     * advancement. New frontends should not submit this directly. */
    EVENT_NONE
} EventType;

typedef struct {
    EventType type;
    union {
        Move move;
        Command command;
        ErrorCode errorCode;
    } data;
} Event;

Event createMoveInputEvent(Command cmd);
Event createPlayerMoveEvent(Move move);
Event createAIMoveEvent(Move move);
Event createSystemEvent(EventType type);
Event createUndoEvent(void);
Event createErrorEvent(ErrorCode code);
Event createFatalErrorEvent(ErrorCode code);

#endif
