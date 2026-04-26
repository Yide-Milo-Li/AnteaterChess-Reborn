#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "ai/ai.h"
#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/movelist.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/movegen.h"
#include "gameplay/move_resolver.h"
#include "gameplay/validation.h"
#include "input/move_request_parser.h"
#include "time/clock.h"

static void clear_board(Board *board) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(board, createPosition(row, col),
                     createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }
}

static void assert_board_equal(const Board *expected, const Board *actual) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece expected_piece = getPiece(expected, createPosition(row, col));
            Piece actual_piece = getPiece(actual, createPosition(row, col));

            assert(expected_piece.type == actual_piece.type);
            assert(expected_piece.color == actual_piece.color);
        }
    }
}

static void assert_state_unchanged(const GameState *before,
                                   const GameState *after) {
    assert_board_equal(&before->board, &after->board);

    assert(before->players[WHITE].type == after->players[WHITE].type);
    assert(before->players[WHITE].color == after->players[WHITE].color);
    assert(before->players[BLACK].type == after->players[BLACK].type);
    assert(before->players[BLACK].color == after->players[BLACK].color);

    assert(before->currentTurn == after->currentTurn);
    assert(before->moveCount == after->moveCount);
    assert(before->gameOver == after->gameOver);
    assert(before->result == after->result);
    assert(before->systemState == after->systemState);

    assert(before->config.mode == after->config.mode);
    assert(before->config.playerColor == after->config.playerColor);
    assert(before->config.aiDifficultyWhite == after->config.aiDifficultyWhite);
    assert(before->config.aiDifficultyBlack == after->config.aiDifficultyBlack);
    assert(before->config.timerEnabled == after->config.timerEnabled);
    assert(before->config.aiTimeLimit == after->config.aiTimeLimit);
    assert(before->config.initialTimeSeconds ==
           after->config.initialTimeSeconds);

    assert(before->moveHistory.count == after->moveHistory.count);
    assert(before->hash == after->hash);
}

static GameState create_ai_ready_state(void) {
    GameConfig config;
    GameState state;

    initDefaultGameConfig(&config);
    config.mode = MODE_COMPUTER_VS_COMPUTER;
    config.aiDifficultyWhite = DIFFICULTY_HARD;
    config.aiDifficultyBlack = DIFFICULTY_HARD;
    config.aiTimeLimit = 1;

    initGameState(&state, &config);
    state.systemState = GAMEPLAY_STATE;
    return state;
}

static int is_promotion_move(SpecialMove type) {
    return isPromotionSpecialMove(type);
}

static void assert_move_is_playable_and_safe(const GameState *state,
                                             Move move) {
    GameState next = *state;
    Color moving_side = state->currentTurn;

    assert(validateMove(state, move) == 1);
    assert(applyMove(&next, move) == 0);
    assert(isInCheck(&next, moving_side) == 0);
}

static void setup_tournament_opening_after_move_six(GameState *state) {
    initBoard(&state->board);
    initMoveList(&state->moveHistory);
    state->moveCount = 0;
    state->currentTurn = WHITE;
    state->config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state->config.aiDifficultyBlack = DIFFICULTY_TOURNAMENT;
    state->config.aiTimeLimit = 0;

    removePiece(&state->board, createPosition(6, 3));
    setPiece(&state->board, createPosition(3, 3), createPiece(ANT, WHITE));
    removePiece(&state->board, createPosition(6, 8));
    setPiece(&state->board, createPosition(5, 8), createPiece(ANT, WHITE));
    removePiece(&state->board, createPosition(0, 1));
    setPiece(&state->board, createPosition(3, 4), createPiece(KNIGHT, BLACK));
    removePiece(&state->board, createPosition(1, 3));
    setPiece(&state->board, createPosition(2, 3), createPiece(ANT, BLACK));
}

static int white_h2_escape_is_blocked(const GameState *state) {
    Piece king = getPiece(&state->board, createPosition(7, 5));
    Piece shield = getPiece(&state->board, createPosition(6, 7));
    Piece blocker = getPiece(&state->board, createPosition(5, 7));

    if (king.type != KING || king.color != WHITE) {
        return 0;
    }
    if (shield.type != ANT || shield.color != WHITE) {
        return 0;
    }
    return blocker.color == WHITE;
}

static void apply_logged_move(GameState *state,
                              const char *fromText,
                              const char *toText) {
    MoveRequest request;
    Move move;

    assert(parseMoveRequestFields(fromText,
        toText,
        PROMOTION_CHOICE_QUEEN,
        &request) == 0);
    assert(resolveMoveRequest(state, request, &move) == 0);
    assert(applyMove(state, move) == 0);
}

static int test_move_allows_immediate_loss(const GameState *state,
                                           Move move,
                                           Color movingSide) {
    GameState *afterMove;
    MoveList *replies;
    int index;

    afterMove = (GameState *)malloc(sizeof(*afterMove));
    assert(afterMove != NULL);
    *afterMove = *state;
    assert(applyMove(afterMove, move) == 0);

    replies = (MoveList *)malloc(sizeof(*replies));
    assert(replies != NULL);
    assert(generateLegalMoves(afterMove, replies) == 0);
    for (index = 0; index < replies->count; ++index) {
        GameState *trial = (GameState *)malloc(sizeof(*trial));
        int loses;

        assert(trial != NULL);
        *trial = *afterMove;
        assert(applyMove(trial, replies->moves[index]) == 0);
        loses = isCheckmate(trial, movingSide);
        free(trial);
        if (loses) {
            free(replies);
            free(afterMove);
            return 1;
        }
    }
    free(replies);
    free(afterMove);
    return 0;
}

static void replay_tournament_log_prefix(GameState *state, int moveCount) {
    static const char *fromSquares[] = {
        "I1", "B8", "D2", "I8", "C1", "D7", "B1", "C7",
        "C3", "B7", "A4", "E7", "E3", "I7", "E2", "G7",
        "H2", "J6", "G2", "H5", "C3", "H8", "A2", "B5",
        "A1", "A6", "D3", "E5", "E2", "C8", "F1", "A6",
        "F2", "A7", "B1", "E8", "I2", "F8", "E1", "G7",
        "C2", "C5", "D4", "B3", "A1", "A8", "A4", "B8",
        "E3", "C4", "G2", "D2", "F2", "E4"
    };
    static const char *toSquares[] = {
        "J3", "A6", "D3", "J6", "E3", "D5", "C3", "C6",
        "A4", "B5", "C3", "E5", "H6", "H6", "E3", "G5",
        "H3", "H5", "G4", "F6", "E2", "I7", "A4", "A4",
        "B1", "C5", "D4", "D4", "D4", "A6", "G2", "C4",
        "F3", "A5", "A1", "E5", "I4", "G7", "D2", "H8",
        "C3", "B3", "F5", "D2", "A4", "B8", "A5", "B2",
        "E4", "F1", "F2", "E4", "E1", "C3"
    };
    int index;

    initBoard(&state->board);
    initMoveList(&state->moveHistory);
    state->moveCount = 0;
    state->currentTurn = WHITE;
    state->config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state->config.aiDifficultyBlack = DIFFICULTY_HARD;
    state->config.aiTimeLimit = 0;

    for (index = 0; index < moveCount; ++index) {
        apply_logged_move(state, fromSquares[index], toSquares[index]);
    }
}

static void setup_tournament_log_after_move_48(GameState *state) {
    replay_tournament_log_prefix(state, 48);
}

static void setup_tournament_log_after_move_54(GameState *state) {
    replay_tournament_log_prefix(state, 54);
}

static void replay_tournament_log_142053_prefix(GameState *state, int moveCount) {
    static const char *fromSquares[] = {
        "D2", "B8", "D4", "C6", "I2", "D7", "I1",
        "C7", "D5", "B7", "B2", "I8", "B1", "H6"
    };
    static const char *toSquares[] = {
        "D4", "C6", "D5", "E5", "I3", "D6", "H3",
        "C6", "C6", "C6", "B4", "H6", "D2", "G4"
    };
    int index;

    initBoard(&state->board);
    initMoveList(&state->moveHistory);
    state->moveCount = 0;
    state->currentTurn = WHITE;
    state->config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state->config.aiDifficultyBlack = DIFFICULTY_HARD;
    state->config.aiTimeLimit = 0;

    for (index = 0; index < moveCount; ++index) {
        apply_logged_move(state, fromSquares[index], toSquares[index]);
    }
}

static void setup_tournament_log_142053_after_move_14(GameState *state) {
    replay_tournament_log_142053_prefix(state, 14);
}

/* Public API should reject invalid output/input pointers. */
static void test_ai_rejects_null_arguments(void) {
    GameState state = create_ai_ready_state();
    Move move;

    assert(generateAIMove(NULL, &move) != 0);
    assert(generateAIMove(&state, NULL) != 0);
    assert(generateAIMoveWithBudget(NULL, &move, 300) != 0);
    assert(generateAIMoveWithBudget(&state, NULL, 300) != 0);
    assert(generateAIMoveWithBudget(&state, &move, 0) != 0);
    assert(generateHintMove(NULL, &move) != 0);
    assert(generateHintMove(&state, NULL) != 0);
}

/* On a normal playable position, the AI should return a legal move and leave
 * the caller-owned GameState untouched. */
static void test_ai_returns_legal_move_without_mutating_initial_state(void) {
    GameState state = create_ai_ready_state();
    GameState before = state;
    Move move;

    assert(generateAIMove(&state, &move) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
}

/* If the side to move starts in check, the AI must choose a move that gets out
 * of check rather than just any pseudo-legal move. */
static void test_ai_resolves_check_with_safe_move(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;

    clear_board(&state.board);
    state.currentTurn = WHITE;

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(7, 0), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(6, 4), createPiece(ANT, WHITE));

    setPiece(&state.board, createPosition(5, 5), createPiece(ROOK, BLACK));
    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));

    before = state;
    assert(isInCheck(&state, WHITE) == 1);
    assert(generateAIMove(&state, &move) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
}

/* Hint generation reuses the AI search engine, so it should obey the same
 * legality and non-mutation guarantees. */
static void test_hint_returns_legal_move_without_mutating_state(void) {
    GameState state = create_ai_ready_state();
    GameState before = state;
    Move move;

    assert(generateHintMove(&state, &move) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
}

static void test_ai_respects_wall_clock_time_limit(void) {
    GameState state = create_ai_ready_state();
    Move move;
    int64_t startMs;
    int64_t endMs;

    assert(getMonotonicMilliseconds(&startMs) == 0);
    assert(generateAIMove(&state, &move) == 0);
    assert(getMonotonicMilliseconds(&endMs) == 0);
    assert(endMs >= startMs);
    assert(endMs - startMs <= 2500);
    assert_move_is_playable_and_safe(&state, move);
}

static void test_ai_generates_move_with_explicit_budget(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;

    state.config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state.config.aiDifficultyBlack = DIFFICULTY_TOURNAMENT;
    state.config.aiTimeLimit = 0;
    before = state;

    assert(generateAIMoveWithBudget(&state, &move, 300) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
}

static void test_tournament_ai_handles_black_king_pressure_with_budget(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;

    clear_board(&state.board);
    state.currentTurn = BLACK;
    state.config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state.config.aiDifficultyBlack = DIFFICULTY_TOURNAMENT;
    state.config.aiTimeLimit = 0;

    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(0, 9), createPiece(ROOK, BLACK));
    setPiece(&state.board, createPosition(1, 4), createPiece(ANT, BLACK));

    setPiece(&state.board, createPosition(3, 5), createPiece(ROOK, WHITE));
    setPiece(&state.board, createPosition(7, 0), createPiece(KING, WHITE));

    before = state;
    assert(isInCheck(&state, BLACK) == 1);
    assert(generateAIMoveWithBudget(&state, &move, 500) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
}

static void test_tournament_ai_prioritizes_stopping_near_promotion(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;
    Position threat = createPosition(6, 5);

    clear_board(&state.board);
    state.currentTurn = WHITE;
    state.config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state.config.aiDifficultyBlack = DIFFICULTY_TOURNAMENT;
    state.config.aiTimeLimit = 0;

    setPiece(&state.board, createPosition(7, 9), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 9), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(6, 0), createPiece(ROOK, WHITE));
    setPiece(&state.board, threat, createPiece(ANT, BLACK));

    before = state;
    assert(generateAIMoveWithBudget(&state, &move, 800) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
    assert(positionEqual(move.to, threat) == 1);
    assert(move.captureCount == 1);
}

static void test_tournament_ai_stops_clear_promotion_runner_early(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;
    Position runner = createPosition(3, 9);

    clear_board(&state.board);
    state.currentTurn = WHITE;
    state.config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state.config.aiDifficultyBlack = DIFFICULTY_TOURNAMENT;
    state.config.aiTimeLimit = 0;

    setPiece(&state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(3, 0), createPiece(ROOK, WHITE));
    setPiece(&state.board, runner, createPiece(ANT, BLACK));

    before = state;
    assert(generateAIMoveWithBudget(&state, &move, 900) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
    assert(positionEqual(move.to, runner) == 1);
    assert(move.captureCount == 1);
}

static void test_tournament_ai_keeps_h2_escape_available_in_opening(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;

    setup_tournament_opening_after_move_six(&state);
    before = state;

    assert(generateAIMoveWithBudget(&state, &move, 800) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
    assert(!(positionEqual(move.from, createPosition(7, 8)) == 1
        && positionEqual(move.to, createPosition(5, 7)) == 1));

    before = state;
    assert(applyMove(&before, move) == 0);
    assert(white_h2_escape_is_blocked(&before) == 0);
}

static void test_tournament_ai_captures_loose_checker_from_logged_game(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;

    setup_tournament_log_after_move_54(&state);
    before = state;

    assert(isInCheck(&state, WHITE) == 1);
    assert(generateAIMoveWithBudget(&state, &move, 7000) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
    assert(!(positionEqual(move.from, createPosition(3, 5)) == 1
        && positionEqual(move.to, createPosition(5, 4)) == 1));
}

static void test_tournament_ai_does_not_ignore_logged_bishop_check_threat(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    Move move;

    setup_tournament_log_after_move_48(&state);
    before = state;

    assert(isInCheck(&state, WHITE) == 0);
    assert(generateAIMoveWithBudget(&state, &move, 1600) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
    assert(!(positionEqual(move.from, createPosition(5, 4)) == 1
        && positionEqual(move.to, createPosition(4, 4)) == 1));
}

static void test_tournament_ai_avoids_logged_h2_mate(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    MoveRequest badRequest;
    Move badMove;
    Move move;

    setup_tournament_log_142053_after_move_14(&state);
    before = state;

    assert(parseMoveRequestFields("H3",
        "I5",
        PROMOTION_CHOICE_QUEEN,
        &badRequest) == 0);
    assert(resolveMoveRequest(&state, badRequest, &badMove) == 0);
    assert(test_move_allows_immediate_loss(&state, badMove, WHITE) == 1);

    assert(generateAIMoveWithBudget(&state, &move, 1200) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);
    assert(test_move_allows_immediate_loss(&state, move, WHITE) == 0);
    assert(!(positionEqual(move.from, createPosition(5, 7)) == 1
        && positionEqual(move.to, createPosition(3, 8)) == 1));
}

static void test_tournament_time_manager_rolls_saved_time_forward(void) {
    AITimeManager manager;
    int firstBudget;
    int secondBudget;
    int cappedBudget;

    initAITimeManager(&manager);
    assert(manager.remainingMs[WHITE] == 600999);
    assert(isAITournamentTimeExpired(&manager, WHITE) == 0);

    firstBudget = getAITournamentBudgetMs(&manager, WHITE);
    assert(firstBudget == 7000);

    updateAITournamentTime(&manager, WHITE, firstBudget, 100);
    assert(manager.remainingMs[WHITE] == 600899);
    assert(manager.poolMs[WHITE] == 6900);

    secondBudget = getAITournamentBudgetMs(&manager, WHITE);
    assert(secondBudget == 10450);

    updateAITournamentTime(&manager, WHITE, secondBudget, 11000);
    assert(manager.poolMs[WHITE] == 6350);

    updateAITournamentTime(&manager, WHITE, 7000, 0);
    updateAITournamentTime(&manager, WHITE, 7000, 0);
    updateAITournamentTime(&manager, WHITE, 7000, 0);
    cappedBudget = getAITournamentBudgetMs(&manager, WHITE);
    assert(cappedBudget == 14000);
    assert(cappedBudget >= 7000);

    manager.remainingMs[WHITE] = 100;
    assert(getAITournamentBudgetMs(&manager, WHITE) == 300);
    updateAITournamentTime(&manager, WHITE, 300, 301);
    assert(manager.remainingMs[WHITE] == 0);
    assert(isAITournamentTimeExpired(&manager, WHITE) == 1);
}

/* Terminal positions with no legal moves should report failure instead of
 * fabricating a move. */
static void test_ai_fails_cleanly_when_no_legal_move_exists(void) {
    GameState state = create_ai_ready_state();
    Move move;

    clear_board(&state.board);
    state.currentTurn = BLACK;

    setPiece(&state.board, createPosition(0, 0), createPiece(KING, BLACK));
    setPiece(&state.board, createPosition(1, 1), createPiece(QUEEN, WHITE));
    setPiece(&state.board, createPosition(2, 2), createPiece(KING, WHITE));

    assert(isCheckmate(&state, BLACK) == 1);
    assert(generateAIMove(&state, &move) != 0);
    assert(generateHintMove(&state, &move) != 0);
}

/* When promotion is the only move family available, the AI should still
 * choose one legal promotion variant. */
static void test_ai_can_choose_promotion_move(void) {
    GameState state = create_ai_ready_state();
    Move move;

    clear_board(&state.board);
    state.currentTurn = WHITE;
    initMoveList(&state.moveHistory);
    state.moveCount = 0;

    setPiece(&state.board, createPosition(1, 2), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(0, 2), createPiece(BISHOP, BLACK));
    setPiece(&state.board, createPosition(0, 3), createPiece(ROOK, BLACK));

    assert(generateAIMove(&state, &move) == 0);
    assert(positionEqual(move.from, createPosition(1, 2)) == 1);
    assert(positionEqual(move.to, createPosition(0, 3)) == 1);
    assert(is_promotion_move(move.specialType) == 1);
}

/* When en passant is the only legal continuation, the AI should emit that
 * special move rather than fabricating an ordinary ant move. */
static void test_ai_can_choose_en_passant(void) {
    GameState state = create_ai_ready_state();
    Move move;

    clear_board(&state.board);
    state.currentTurn = WHITE;
    initMoveList(&state.moveHistory);
    state.moveCount = 0;

    setPiece(&state.board, createPosition(3, 4), createPiece(ANT, WHITE));
    setPiece(&state.board, createPosition(3, 5), createPiece(ANT, BLACK));
    setPiece(&state.board, createPosition(2, 4), createPiece(ROOK, BLACK));
    assert(addMove(&state.moveHistory,
        createMove(createPosition(1, 5), createPosition(3, 5), createPiece(ANT, BLACK))) == 0);
    state.moveCount = 1;

    assert(generateAIMove(&state, &move) == 0);
    assert(positionEqual(move.from, createPosition(3, 4)) == 1);
    assert(positionEqual(move.to, createPosition(2, 5)) == 1);
    assert(move.specialType == EN_PASSANT);
}

int main(void) {
    test_ai_rejects_null_arguments();
    test_ai_returns_legal_move_without_mutating_initial_state();
    test_ai_resolves_check_with_safe_move();
    test_hint_returns_legal_move_without_mutating_state();
    test_ai_respects_wall_clock_time_limit();
    test_ai_generates_move_with_explicit_budget();
    test_tournament_ai_handles_black_king_pressure_with_budget();
    test_tournament_ai_prioritizes_stopping_near_promotion();
    test_tournament_ai_stops_clear_promotion_runner_early();
    test_tournament_ai_keeps_h2_escape_available_in_opening();
    test_tournament_ai_captures_loose_checker_from_logged_game();
    test_tournament_ai_does_not_ignore_logged_bishop_check_threat();
    test_tournament_ai_avoids_logged_h2_mate();
    test_tournament_time_manager_rolls_saved_time_forward();
    test_ai_fails_cleanly_when_no_legal_move_exists();
    test_ai_can_choose_promotion_move();
    test_ai_can_choose_en_passant();
    return 0;
}
