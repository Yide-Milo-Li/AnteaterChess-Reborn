#ifndef AC_AI_INTERNAL_H
#define AC_AI_INTERNAL_H
#include "anteater/ai.hpp"
#include "score_constants.hpp"
#include "evaluation.hpp"
#include "piece_tables.hpp"
#include "move_facts.hpp"
#include "see.hpp"
#include "../rules/internal.hpp"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

namespace ac {
#define AC_TT_SIZE (1U << 20)
struct HashState {
    uint64_t value;
};
#define AI_Q_DEPTH 8           // Quiescence search depth
#define AI_MAX_PLY 48          // Maximum number of ply
#define AI_HISTORY_MAX 2000000 // upper limit of history heuristic's score
#define ASPIRATION_WINDOW 60
#define NULL_MOVE_R 2

enum { TT_FLAG_EXACT = 0, TT_FLAG_LOWER = 1, TT_FLAG_UPPER = 2 };
struct TTEntry {
    uint64_t key; // position's zobrist hash
    int score;
    short depth;
    unsigned char flag;       // TT_FLAG_EXACT, TT_FLAG_LOWER, TT_FLAG_UPPER
    unsigned char generation; // search version
    unsigned char from;       // best move from
    unsigned char to;         // best move to
    unsigned char special;    // special move flag
};

struct SearchOptions {
    Clock clock{};
    int budgetMs = 0, maxDepth = 0;
    std::stop_token stop;
    std::span<const uint64_t> hashes;
};
namespace detail {
struct SearchData {
    explicit SearchData(std::pmr::memory_resource *resource) : moveBuffers(resource), transpositionTable(resource) {
        transpositionTable.resize(AC_TT_SIZE);
        moveBuffers.resize(AI_MAX_PLY + 1);
    }
    int64_t searchStartMs{};
    int timeLimitMs{};
    int softTimeLimitMs{};
    int stopSearch{};
    int nodes{};
    unsigned char generation{};
    detail::OwnedSequence<MoveList> moveBuffers;
    HashState hashStack[AI_MAX_PLY + 1]{}; // Length 49
    uint64_t gameHashes[MaxMoves + 1]{};
    int gameHashCount{};
    int gameHistoryStart{};
    int repetitionLimit[AI_MAX_PLY + 1]{};
    unsigned char nullMoveActive[AI_MAX_PLY + 1]{};
    detail::OwnedSequence<TTEntry> transpositionTable;
    unsigned char ttGeneration{};
    Move killerMoves[AI_MAX_PLY][2]{};
    unsigned char killerValid[AI_MAX_PLY][2]{};
    int history[2][Rows * Columns][Rows * Columns]{};
    unsigned char searchHeuristicsReady{};
    Undo undoStack[AI_MAX_PLY + 1]{};
    SearchOptions options{};
    Status failure{};
    int completedDepth{};
};
} // namespace detail
using detail::SearchData;

static inline Status read_clock(const SearchData *ctx, int64_t *out) {
    if (!ctx || !ctx->options.clock.now)
        return Status::InvalidArgument;
    *out = ctx->options.clock.now(ctx->options.clock.context);
    return Status::Ok;
}
static inline int derive_hash(const Position *s, HashState *h) {
    h->value = s->hash;
    return 0;
}
int ai_elapsed_ms(const SearchData *ctx);
int ai_time_is_up(SearchData *ctx);
void ai_ensure_search_heuristics_ready(SearchData *ctx);
void ai_age_history_scores(SearchData *ctx);
Status ai_init_search_context(SearchData *ctx, int timeLimitMs);
int ai_build_game_hash_history(SearchData *ctx, const Position *state);
int ai_node_is_repetition(const SearchData *ctx, uint64_t key, int ply);
TTEntry *ai_tt_slot(SearchData *ctx, uint64_t key);
int ai_score_to_tt(int score, int ply);
int ai_score_from_tt(int score, int ply);
int ai_tt_lookup(SearchData *ctx, uint64_t key, TTEntry *entry);
void ai_tt_store(SearchData *ctx, uint64_t key, int depth, int ply, int score, int flag, const Move *bestMove,
                 unsigned char generation);
int ai_move_matches_entry(const Move *move, const TTEntry *entry);
int ai_move_equal_signature(const Move *lhs, const Move *rhs);
void ai_save_killer(SearchData *ctx, int ply, Move move);
void ai_update_history_score(SearchData *ctx, Color color, Move move, int delta);
int ai_tactical_move_score(const Move *move);
int ai_quick_exchange_margin(const Move *move);
int ai_move_order_score(SearchData *ctx, const Position *state, const Move *move, int ply, const TTEntry *ttMove);
void ai_sort_moves(SearchData *ctx, const Position *state, MoveList *list, int ply, const TTEntry *ttMove);
void ai_sort_root_moves_by_scores(MoveList *list, int scores[MaxMoves]);
Status ai_generate_search_moves(Position *state, MoveList *list, int onlyNoisy);
int ai_side_has_major_material(const Board *board, Color color);
int ai_try_null_move(SearchData *ctx, Position *state, int depth, int beta, int ply);
int ai_alpha_beta(SearchData *ctx, Position *state, int depth, int alpha, int beta, int ply, int allowNull);
int ai_quiescence(SearchData *ctx, Position *state, int alpha, int beta, int ply, int qDepth);
int ai_search_best_move(SearchData *ctx, const Position *state, int maxDepth, int maxTimeMs, Move *bestMove);

} // namespace ac
#endif
