#include <assert.h>
#include <string.h>

#include "error/error.h"
#include "error/error_code.h"

/* Check that each public error code maps to the documented user-facing message. */
static void test_known_error_messages(void) {
    assert(getErrorMessage(ERR_INVALID_INPUT) != NULL);
    assert(getErrorMessage(ERR_INVALID_INPUT) == getErrorMessage(ERR_INVALID_INPUT));

    assert(strcmp(getErrorMessage(ERR_INVALID_INPUT), "Invalid input.") == 0);
    assert(strcmp(getErrorMessage(ERR_INVALID_MENU_SELECTION), "Choose a listed option.") == 0);
    assert(strcmp(getErrorMessage(ERR_INVALID_MOVE_FORMAT), "Use move format: E2 E4.") == 0);
    assert(strcmp(getErrorMessage(ERR_POSITION_OUT_OF_BOUNDS), "Position is out of bounds.") == 0);
    assert(strcmp(getErrorMessage(ERR_EMPTY_SELECTION), "No piece selected.") == 0);
    assert(strcmp(getErrorMessage(ERR_OPPONENT_PIECE), "That piece is not yours.") == 0);
    assert(strcmp(getErrorMessage(ERR_ILLEGAL_MOVE), "Illegal move.") == 0);
    assert(strcmp(getErrorMessage(ERR_UNRESOLVED_CHECK), "Move leaves king in check.") == 0);
    assert(strcmp(getErrorMessage(ERR_INVALID_TIMER_SETTING), "Enter a time from 1 to 3600.") == 0);
    assert(strcmp(getErrorMessage(ERR_UNDO_UNAVAILABLE), "Undo unavailable.") == 0);
    assert(strcmp(getErrorMessage(ERR_HINT_UNAVAILABLE), "Hint unavailable.") == 0);
    assert(strcmp(getErrorMessage(ERR_NOT_YOUR_TURN), "Not your turn.") == 0);
    assert(strcmp(getErrorMessage(ERR_TIME_UP), "Time is up.") == 0);
    assert(strcmp(getErrorMessage(ERR_FATAL), "Fatal error.") == 0);
}

/* Check that unknown codes return the shared fallback message. */
static void test_unknown_error_message(void) {
    assert(strcmp(getErrorMessage((ErrorCode)999), "Unknown error.") == 0);
    assert(strcmp(getErrorMessage((ErrorCode)(-1)), "Unknown error.") == 0);
}

/* Run the minimal Phase J error regression suite. */
int main(void) {
    test_known_error_messages();
    test_unknown_error_message();
    return 0;
}
