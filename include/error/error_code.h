#ifndef CHESS_ERROR_ERROR_CODE_H
#define CHESS_ERROR_ERROR_CODE_H

typedef enum {
    ERR_INVALID_INPUT,
    /* Header change: CLI flows need a separate code for a parsed number that
     * does not match any visible menu choice. */
    ERR_INVALID_MENU_SELECTION,
    /* Header change: move-entry callers need one stable code for malformed
     * FROM/TO text without conflating it with move legality. */
    ERR_INVALID_MOVE_FORMAT,
    /* Header change: current validation already detects off-board positions,
     * so the public enum needs a dedicated user-facing result. */
    ERR_POSITION_OUT_OF_BOUNDS,
    ERR_EMPTY_SELECTION,
    ERR_OPPONENT_PIECE,
    ERR_ILLEGAL_MOVE,
    ERR_UNRESOLVED_CHECK,
    /* Header change: setup flows validate turn-length input separately from
     * generic menu parsing, so the public enum needs a timer-specific code. */
    ERR_INVALID_TIMER_SETTING,
    ERR_INVALID_AI_TIMER_SETTING,
    ERR_UNDO_UNAVAILABLE,
    ERR_HINT_UNAVAILABLE,
    ERR_AI_UNAVAILABLE,
    ERR_NOT_YOUR_TURN,
    ERR_TIME_UP,
    ERR_ACTION_UNAVAILABLE,
    ERR_FATAL
} ErrorCode;

#endif
