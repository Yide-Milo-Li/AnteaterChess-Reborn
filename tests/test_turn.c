#include <assert.h>
#include <stddef.h>

#include "core/gamestate.h"
#include "turn/turn.h"

void test_switch_turn_toggles_between_white_and_black(void) {
    GameState state;

    initGameState(&state, NULL);
    assert(getCurrentTurn(&state) == WHITE);

    assert(switchTurn(&state) == 0);
    assert(getCurrentTurn(&state) == BLACK);

    assert(switchTurn(&state) == 0);
    assert(getCurrentTurn(&state) == WHITE);
}

void test_switch_turn_rejects_invalid_state(void) {
    GameState state;

    initGameState(&state, NULL);
    state.currentTurn = EMPTY_COLOR;

    assert(switchTurn(NULL) == 1);
    assert(switchTurn(&state) == 1);
    assert(getCurrentTurn(NULL) == EMPTY_COLOR);
    assert(getCurrentTurn(&state) == EMPTY_COLOR);
}

void test_is_player_turn_reports_matching_color(void) {
    GameState state;

    initGameState(&state, NULL);
    assert(isPlayerTurn(&state, WHITE) == 1);
    assert(isPlayerTurn(&state, BLACK) == 0);

    state.currentTurn = BLACK;
    assert(isPlayerTurn(&state, WHITE) == 0);
    assert(isPlayerTurn(&state, BLACK) == 1);

    assert(isPlayerTurn(NULL, WHITE) == 0);
    assert(isPlayerTurn(&state, EMPTY_COLOR) == 0);
}

void test_restore_turn_after_undo_uses_remaining_history_count(void) {
    GameState state;
    Move whiteMove;
    Move blackMove;

    initGameState(&state, NULL);
    state.currentTurn = BLACK;

    assert(restoreTurnAfterUndo(&state) == 0);
    assert(state.currentTurn == WHITE);

    whiteMove = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));
    blackMove = createMove(createPosition(1, 4), createPosition(2, 4), createPiece(ANT, BLACK));

    assert(addMoveToHistory(&state, whiteMove) == 0);
    state.currentTurn = WHITE;
    assert(restoreTurnAfterUndo(&state) == 0);
    assert(state.currentTurn == BLACK);

    assert(addMoveToHistory(&state, blackMove) == 0);
    state.currentTurn = BLACK;
    assert(restoreTurnAfterUndo(&state) == 0);
    assert(state.currentTurn == WHITE);
}

int main(void) {
    test_switch_turn_toggles_between_white_and_black();
    test_switch_turn_rejects_invalid_state();
    test_is_player_turn_reports_matching_color();
    test_restore_turn_after_undo_uses_remaining_history_count();
    return 0;
}
