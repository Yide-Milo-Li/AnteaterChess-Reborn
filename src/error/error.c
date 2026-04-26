#include "error/error.h"

/*
 * Alignment assumptions for future extensions:
 * - error.h and error_code.h are the truth source for this module's public contract.
 * - This file only maps ErrorCode values to stable user-facing messages.
 * - Richer error context, payload, and GUI presentation remain outside this module for now.
 */

/* Return the stable user-facing message for one public error code. */
const char *getErrorMessage(ErrorCode code) {
    switch (code) {
        case ERR_INVALID_INPUT:
            return "Invalid input.";
        case ERR_INVALID_MENU_SELECTION:
            return "Choose a listed option.";
        case ERR_INVALID_MOVE_FORMAT:
            return "Use move format: E2 E4.";
        case ERR_POSITION_OUT_OF_BOUNDS:
            return "Position is out of bounds.";
        case ERR_EMPTY_SELECTION:
            return "No piece selected.";
        case ERR_OPPONENT_PIECE:
            return "That piece is not yours.";
        case ERR_ILLEGAL_MOVE:
            return "Illegal move.";
        case ERR_UNRESOLVED_CHECK:
            return "Move leaves king in check.";
        case ERR_INVALID_TIMER_SETTING:
            return "Enter a time from 1 to 3600.";
        case ERR_UNDO_UNAVAILABLE:
            return "Undo unavailable.";
        case ERR_HINT_UNAVAILABLE:
            return "Hint unavailable.";
        case ERR_AI_UNAVAILABLE:
            return "AI move unavailable.";
        case ERR_NOT_YOUR_TURN:
            return "Not your turn.";
        case ERR_TIME_UP:
            return "Time is up.";
        case ERR_ACTION_UNAVAILABLE:
            return "Action unavailable.";
        case ERR_FATAL:
            return "Fatal error.";
        default:
            return "Unknown error.";
    }
}
