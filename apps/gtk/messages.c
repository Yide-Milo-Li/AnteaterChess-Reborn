#include "gui_internal.h"

/*
 * Alignment assumptions for future extensions:
 * - error.h and error_code.h are the truth source for this module's public contract.
 * - This file only maps AcErrorCode values to stable user-facing messages.
 * - Richer error context, payload, and GUI presentation remain outside this module for now.
 */

/* Return the stable user-facing message for one public error code. */
const char *ac_get_error_message(AcErrorCode code) {
    switch (code) {
    case AC_ERR_INVALID_INPUT:
        return "Invalid input.";
    case AC_ERR_INVALID_MENU_SELECTION:
        return "Choose a listed option.";
    case AC_ERR_INVALID_MOVE_FORMAT:
        return "Use move format: E2 E4.";
    case AC_ERR_POSITION_OUT_OF_BOUNDS:
        return "AcSquare is out of bounds.";
    case AC_ERR_EMPTY_SELECTION:
        return "No piece selected.";
    case AC_ERR_OPPONENT_PIECE:
        return "That piece is not yours.";
    case AC_ERR_ILLEGAL_MOVE:
        return "Illegal move.";
    case AC_ERR_UNRESOLVED_CHECK:
        return "AcMove leaves king in check.";
    case AC_ERR_INVALID_TIMER_SETTING:
        return "Enter a time from 1 to 3600.";
    case AC_ERR_INVALID_AI_TIMER_SETTING:
        return "AC_AI difficulty requires a longer turn timer.";
    case AC_ERR_UNDO_UNAVAILABLE:
        return "Undo unavailable.";
    case AC_ERR_HINT_UNAVAILABLE:
        return "Hint unavailable.";
    case AC_ERR_AI_UNAVAILABLE:
        return "AC_AI move unavailable.";
    case AC_ERR_NOT_YOUR_TURN:
        return "Not your turn.";
    case AC_ERR_TIME_UP:
        return "Time is up.";
    case AC_ERR_ACTION_UNAVAILABLE:
        return "Action unavailable.";
    case AC_ERR_FATAL:
        return "Fatal error.";
    default:
        return "Unknown error.";
    }
}
