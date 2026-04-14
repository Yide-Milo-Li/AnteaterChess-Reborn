#ifndef CHESS_SYSTEM_EVENT_H
#define CHESS_SYSTEM_EVENT_H

#include "core/move.h"
#include "error/error_code.h"
#include "input/command.h"

typedef enum {
    EVENT_MOVE_INPUT,
    EVENT_AI_MOVE,
    EVENT_UNDO,
    EVENT_TIMER_EXPIRED,
    EVENT_LEAVE_GAME,
    EVENT_BACK,
    EVENT_NEW_GAME,
    EVENT_EXIT_PROGRAM,
    EVENT_ERROR,
    EVENT_FATAL_ERROR,
    EVENT_HINT,
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
Event createAIMoveEvent(Move move);
Event createSystemEvent(EventType type);
Event createUndoEvent(void);
Event createErrorEvent(ErrorCode code);
Event createFatalErrorEvent(ErrorCode code);

#endif
