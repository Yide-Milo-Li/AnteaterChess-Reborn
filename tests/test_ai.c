#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include "ai/ai.h"
#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/movelist.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/validation.h"
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

static void setup_tournament_knight_jump_position(GameState *state) {
    clear_board(&state->board);
    state->currentTurn = WHITE;
    state->config.aiDifficultyWhite = DIFFICULTY_TOURNAMENT;
    state->config.aiDifficultyBlack = DIFFICULTY_TOURNAMENT;
    state->config.aiTimeLimit = 0;

    setPiece(&state->board, createPosition(7, 0), createPiece(ROOK, WHITE));
    setPiece(&state->board, createPosition(7, 2), createPiece(BISHOP, WHITE));
    setPiece(&state->board, createPosition(7, 3), createPiece(ANTEATER, WHITE));
    setPiece(&state->board, createPosition(7, 4), createPiece(QUEEN, WHITE));
    setPiece(&state->board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state->board, createPosition(7, 6), createPiece(ANTEATER, WHITE));
    setPiece(&state->board, createPosition(7, 7), createPiece(BISHOP, WHITE));
    setPiece(&state->board, createPosition(7, 9), createPiece(ROOK, WHITE));
    setPiece(&state->board, createPosition(6, 0), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(6, 2), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(6, 3), createPiece(KNIGHT, WHITE));
    setPiece(&state->board, createPosition(6, 4), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(6, 5), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(6, 6), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(6, 7), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(6, 9), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(5, 7), createPiece(KNIGHT, WHITE));
    setPiece(&state->board, createPosition(5, 8), createPiece(ANT, WHITE));
    setPiece(&state->board, createPosition(4, 1), createPiece(ANT, WHITE));

    setPiece(&state->board, createPosition(0, 0), createPiece(ROOK, BLACK));
    setPiece(&state->board, createPosition(0, 2), createPiece(BISHOP, BLACK));
    setPiece(&state->board, createPosition(0, 3), createPiece(ANTEATER, BLACK));
    setPiece(&state->board, createPosition(0, 4), createPiece(QUEEN, BLACK));
    setPiece(&state->board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state->board, createPosition(0, 6), createPiece(ANTEATER, BLACK));
    setPiece(&state->board, createPosition(0, 7), createPiece(BISHOP, BLACK));
    setPiece(&state->board, createPosition(0, 9), createPiece(ROOK, BLACK));
    setPiece(&state->board, createPosition(1, 0), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(1, 4), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(1, 5), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(1, 6), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(1, 7), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(1, 8), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(1, 9), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(2, 2), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(2, 3), createPiece(ANT, BLACK));
    setPiece(&state->board, createPosition(3, 4), createPiece(KNIGHT, BLACK));
    setPiece(&state->board, createPosition(4, 6), createPiece(KNIGHT, BLACK));
}

static int black_knight_h2_mate_available(const GameState *state) {
    GameState trial;
    Move move;
    Position from = createPosition(4, 6);
    Position to = createPosition(6, 7);
    Piece target;

    trial = *state;
    move = createMove(from, to, createPiece(KNIGHT, BLACK));
    target = getPiece(&trial.board, to);
    if (target.color == WHITE) {
        addCapture(&move, to, target);
    }

    if (validateMove(&trial, move) != 1) {
        return 0;
    }
    if (applyMove(&trial, move) != 0) {
        return 0;
    }
    return isCheckmate(&trial, WHITE);
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

static void test_tournament_ai_defuses_near_king_knight_jump(void) {
    GameState state = create_ai_ready_state();
    GameState before;
    GameState after;
    Move move;

    setup_tournament_knight_jump_position(&state);
    before = state;

    assert(generateAIMoveWithBudget(&state, &move, 1200) == 0);
    assert_state_unchanged(&before, &state);
    assert_move_is_playable_and_safe(&state, move);

    after = state;
    assert(applyMove(&after, move) == 0);
    assert(black_knight_h2_mate_available(&after) == 0);
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
    assert(secondBudget == 8725);

    updateAITournamentTime(&manager, WHITE, secondBudget, 11000);
    assert(manager.poolMs[WHITE] == 4625);

    updateAITournamentTime(&manager, WHITE, 7000, 0);
    updateAITournamentTime(&manager, WHITE, 7000, 0);
    updateAITournamentTime(&manager, WHITE, 7000, 0);
    cappedBudget = getAITournamentBudgetMs(&manager, WHITE);
    assert(cappedBudget <= 10000);
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
    test_tournament_ai_defuses_near_king_knight_jump();
    test_tournament_time_manager_rolls_saved_time_forward();
    test_ai_fails_cleanly_when_no_legal_move_exists();
    test_ai_can_choose_promotion_move();
    test_ai_can_choose_en_passant();
    return 0;
}
