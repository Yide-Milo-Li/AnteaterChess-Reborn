#include <assert.h>
#include <stddef.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"

static void assertPlayer(Player player, Color color, PlayerType type) {
    assert(getPlayerColor(player) == color);
    assert(isHumanPlayer(player) == (type == HUMAN));
    assert(isAIPlayer(player) == (type == AI));
}

static void test_game_state_initialization_defaults(void) {
    GameState state;

    initGameState(&state, NULL);

    assert(getPiece(&state.board, createPosition(0, 0)).type == ROOK);
    assert(getPiece(&state.board, createPosition(7, 5)).type == KING);
    assert(getPiece(&state.board, createPosition(3, 4)).type == EMPTY_PIECE);
    assertPlayer(state.players[WHITE], WHITE, HUMAN);
    assertPlayer(state.players[BLACK], BLACK, HUMAN);
    assert(state.currentTurn == WHITE);
    assert(getCurrentPlayer(&state) == &state.players[WHITE]);
    assert(state.moveCount == 0);
    assert(getMoveHistory(&state) == &state.moveHistory);
    assert(state.moveHistory.count == 0);
    assert(isGameOver(&state) == 0);
    assert(getGameResult(&state) == RESULT_NONE);
    assert(state.systemState == INIT_STATE);
    assert(state.config.mode == MODE_HUMAN_VS_HUMAN);
    assert(state.hash == 0);
}

static void test_game_state_player_setup_follows_mode(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    initGameState(&state, &config);
    assertPlayer(state.players[WHITE], WHITE, AI);
    assertPlayer(state.players[BLACK], BLACK, HUMAN);

    config.mode = MODE_COMPUTER_VS_COMPUTER;
    initGameState(&state, &config);
    assertPlayer(state.players[WHITE], WHITE, AI);
    assertPlayer(state.players[BLACK], BLACK, AI);
}

static void test_history_and_result_helpers(void) {
    GameState state;
    Move move;

    initGameState(&state, NULL);
    move = createMove(createPosition(6, 4), createPosition(5, 4), createPiece(ANT, WHITE));

    assert(addMoveToHistory(&state, move) == 0);
    assert(state.moveCount == 1);
    assert(state.moveHistory.count == 1);
    assert(positionEqual(state.moveHistory.moves[0].from, createPosition(6, 4)) == 1);
    assert(positionEqual(state.moveHistory.moves[0].to, createPosition(5, 4)) == 1);

    assert(removeLastMoveFromHistory(&state) == 0);
    assert(state.moveCount == 0);
    assert(state.moveHistory.count == 0);
    assert(removeLastMoveFromHistory(&state) == 1);

    setGameResult(&state, RESULT_DRAW);
    assert(getGameResult(&state) == RESULT_DRAW);
    assert(isGameOver(&state) == 1);

    setGameResult(&state, RESULT_NONE);
    assert(isGameOver(&state) == 0);

    setGameOver(&state);
    assert(isGameOver(&state) == 1);
}

static void test_get_current_player_tracks_turn(void) {
    GameState state;

    initGameState(&state, NULL);
    assert(getCurrentPlayer(&state) == &state.players[WHITE]);

    state.currentTurn = BLACK;
    assert(getCurrentPlayer(&state) == &state.players[BLACK]);

    state.currentTurn = EMPTY_COLOR;
    assert(getCurrentPlayer(&state) == NULL);
}

int main(void) {
    test_game_state_initialization_defaults();
    test_game_state_player_setup_follows_mode();
    test_history_and_result_helpers();
    test_get_current_player_tracks_turn();
    return 0;
}
