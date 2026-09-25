#ifndef AC_AI_INTERNAL_H
#define AC_AI_INTERNAL_H
#include "anteater/ai.h"
#include "../rules/internal.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define AC_TT_SIZE (1U << 20)
typedef struct {
    uint64_t value;
} AcHashState;
#define AI_INF 100000000       // Infinity
#define AI_MATE 1000000        // Mate score
#define AI_Q_DEPTH 8           // Quiescence search depth
#define AI_MAX_PLY 48          // Maximum number of ply
#define AI_MAX_PHASE 28        // 28 for initial, 0 for final stage
#define AI_HISTORY_MAX 2000000 // upper limit of history heuristic's score
#define ASPIRATION_WINDOW 60
#define NULL_MOVE_R 2
#define AI_TOURNAMENT_TOTAL_MS 600999
#define AI_TOURNAMENT_RESERVE_MS 30000
#define AI_TOURNAMENT_BASE_MS 7000
#define AI_TOURNAMENT_MAX_MS 10000
#define AI_TOURNAMENT_MAX_EXTRA_MS 3000
#define AI_TOURNAMENT_POOL_CAP_MS 180000
#define AI_MIN_MOVE_BUDGET_MS 300

enum { TT_FLAG_EXACT = 0, TT_FLAG_LOWER = 1, TT_FLAG_UPPER = 2 };
typedef struct {
    uint64_t key; // position's zobrist hash
    int score;
    short depth;
    unsigned char flag;       // TT_FLAG_EXACT, TT_FLAG_LOWER, TT_FLAG_UPPER
    unsigned char generation; // search version
    unsigned char from;       // best move from
    unsigned char to;         // best move to
    unsigned char special;    // special move flag
} TTEntry;

struct AcSearchContext {
    int64_t searchStartMs;
    int timeLimitMs;
    int softTimeLimitMs;
    int stopSearch;
    int nodes;
    unsigned char generation;
    AcMoveList *moveBuffers;
    AcHashState hashStack[AI_MAX_PLY + 1]; // Length 49
    uint64_t gameHashes[AC_MAX_MOVES + 1];
    int gameHashCount;
    int gameHistoryStart;
    int repetitionLimit[AI_MAX_PLY + 1];
    unsigned char nullMoveActive[AI_MAX_PLY + 1];
    TTEntry *transpositionTable;
    unsigned char ttGeneration;
    AcMove killerMoves[AI_MAX_PLY][2];
    unsigned char killerValid[AI_MAX_PLY][2];
    int history[2][AC_ROWS * AC_COLS][AC_ROWS * AC_COLS];
    unsigned char searchHeuristicsReady;
    AcUndo undoStack[AI_MAX_PLY + 1];
    AcSearchOptions options;
    AcStatus failure;
    int completedDepth;
};

static inline int read_clock(const AcSearchContext *ctx, int64_t *out) {
    if (!ctx || !ctx->options.clock.now)
        return AC_INVALID_ARGUMENT;
    *out = ctx->options.clock.now(ctx->options.clock.context);
    return AC_OK;
}
static inline int derive_hash(const AcPosition *s, AcHashState *h) {
    h->value = s->hash;
    return 0;
}
int ac_ai_piece_value(AcPieceType type);
int ac_ai_phase_value(AcPieceType type);
int ac_ai_mobility_weight(AcPieceType type);
int ac_ai_board_row_for_pst(AcColor color, int row);
int ac_ai_king_pst_bonus(AcPiece piece, int row, int col, int phase);
int ac_ai_pst_bonus(AcPiece piece, int row, int col, int phase);
int ac_ai_is_promotion_move(const AcMove *move);
int ac_ai_is_noisy_move(const AcMove *move);
int ac_ai_is_quiet_move(const AcMove *move);
int ac_ai_square_index(AcSquare pos);
int ac_ai_absolute_value(int value);
int ac_ai_is_irreversible_move(const AcMove *move);
int ac_ai_is_path_clear_for_attack(const AcBoard *board, AcSquare from, AcSquare target);
int ac_ai_ant_attacks_square(AcSquare from, AcPiece piece, AcSquare target);
int ac_ai_rook_attacks_square(const AcBoard *board, AcSquare from, AcSquare target);
int ac_ai_bishop_attacks_square(const AcBoard *board, AcSquare from, AcSquare target);
int ac_ai_knight_attacks_square(AcSquare from, AcSquare target);
int ac_ai_king_attacks_square(AcSquare from, AcSquare target);
int ac_ai_anteater_attacks_piece_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target,
                                         AcPiece targetPiece);
int ac_ai_piece_attacks_square_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target,
                                       AcPiece targetPiece);
int ac_ai_square_is_attacked_for_king(const AcBoard *board, AcSquare target, AcColor attackingColor);
AcPiece ac_ai_piece_after_see_capture(AcPiece piece, AcSquare target);
void ac_ai_apply_move_to_board_for_see(AcBoard *board, AcMove move);
int ac_ai_king_capture_is_legal_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target);
int ac_ai_find_least_valuable_attacker(const AcBoard *board, AcSquare target, AcColor side, AcSquare *fromOut,
                                       AcPiece *pieceOut, int *valueOut);
int ac_ai_see_initial_gain(const AcMove *move);
int ac_ai_see_move_score(const AcPosition *state, const AcMove *move);
int ac_ai_count_sliding_mobility(const AcBoard *board, AcSquare from, AcPiece piece, int rowStep, int colStep);
int ac_ai_count_ant_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
int ac_ai_count_knight_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
int ac_ai_count_anteater_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
int ac_ai_count_king_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
int ac_ai_piece_mobility(const AcBoard *board, AcSquare from, AcPiece piece);
int ac_ai_ant_supports_square(const AcBoard *board, AcColor color, AcSquare target);
int ac_ai_enemy_ant_can_attack_square(const AcBoard *board, AcColor color, AcSquare target);
int ac_ai_evaluate_pawns(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS], int phase);
int ac_ai_evaluate_rook_and_queen_files(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS],
                                        int phase);
int ac_ai_evaluate_king_safety(const AcPosition *state, AcColor color, AcSquare kingPos, int phase);
int ac_ai_evaluate_development(const AcPosition *state, AcColor color, int phase);
int ac_ai_evaluate_anteater_threats(const AcBoard *board, AcColor color);
int ac_ai_evaluate_outposts(const AcBoard *board, AcColor color, int phase);
int ac_ai_evaluate_absolute(const AcPosition *state);
int ac_ai_evaluate_relative(const AcPosition *state);
int ac_ai_clamp_int(int value, int minValue, int maxValue);
int ac_ai_color_time_index(AcColor color);
int ac_ai_elapsed_ms(const AcSearchContext *ctx);
int ac_ai_time_is_up(AcSearchContext *ctx);
void ac_init_ai_time_manager(AcAITimeManager *manager);
int ac_get_ai_tournament_budget_ms(AcAITimeManager *manager, AcColor color);
int ac_is_ai_tournament_time_expired(const AcAITimeManager *manager, AcColor color);
void ac_update_ai_tournament_time(AcAITimeManager *manager, AcColor color, int budgetMs, int elapsedMs);
void ac_ai_ensure_search_heuristics_ready(AcSearchContext *ctx);
void ac_ai_age_history_scores(AcSearchContext *ctx);
int ac_ai_init_search_context(AcSearchContext *ctx, int timeLimitMs);
int ac_ai_build_game_hash_history(AcSearchContext *ctx, const AcPosition *state);
int ac_ai_node_is_repetition(const AcSearchContext *ctx, uint64_t key, int ply);
TTEntry *ac_ai_tt_slot(AcSearchContext *ctx, uint64_t key);
int ac_ai_score_to_tt(int score, int ply);
int ac_ai_score_from_tt(int score, int ply);
int ac_ai_tt_lookup(AcSearchContext *ctx, uint64_t key, TTEntry *entry);
void ac_ai_tt_store(AcSearchContext *ctx, uint64_t key, int depth, int ply, int score, int flag, const AcMove *bestMove,
                    unsigned char generation);
int ac_ai_move_matches_entry(const AcMove *move, const TTEntry *entry);
int ac_ai_move_equal_signature(const AcMove *lhs, const AcMove *rhs);
void ac_ai_save_killer(AcSearchContext *ctx, int ply, AcMove move);
void ac_ai_update_history_score(AcSearchContext *ctx, AcColor color, AcMove move, int delta);
int ac_ai_tactical_move_score(const AcMove *move);
int ac_ai_quick_exchange_margin(const AcMove *move);
int ac_ai_move_order_score(AcSearchContext *ctx, const AcPosition *state, const AcMove *move, int ply,
                           const TTEntry *ttMove);
void ac_ai_sort_moves(AcSearchContext *ctx, const AcPosition *state, AcMoveList *list, int ply, const TTEntry *ttMove);
void ac_ai_sort_root_moves_by_scores(AcMoveList *list, int scores[AC_MAX_MOVES]);
int ac_ai_generate_search_moves(AcPosition *state, AcMoveList *list, int onlyNoisy);
int ac_ai_side_has_major_material(const AcBoard *board, AcColor color);
int ac_ai_try_null_move(AcSearchContext *ctx, AcPosition *state, int depth, int beta, int ply);
int ac_ai_alpha_beta(AcSearchContext *ctx, AcPosition *state, int depth, int alpha, int beta, int ply, int allowNull);
int ac_ai_quiescence(AcSearchContext *ctx, AcPosition *state, int alpha, int beta, int ply, int qDepth);
int ac_search_depth(AcAIDifficulty difficulty);
int ac_ai_search_best_move(AcSearchContext *ctx, const AcPosition *state, int maxDepth, int maxTimeMs,
                           AcMove *bestMove);
#endif
