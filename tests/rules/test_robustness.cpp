#include "anteater/rules.hpp"
#include "anteater/session.hpp"
#include "anteater/ai.hpp"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace ac;

/* Simple LCG pseudo-random number generator for reproducible fuzzing */
static uint32_t fuzz_rand(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

/* 1. Audit Coordinate and Move Request Parser Robustness against Malformed Inputs */
static void audit_parser_fuzzing(void) {
    uint32_t rng = 0xDEADBEEF;
    MoveRequest req;
    char buffer[128];

    /* Basic boundary inputs */
    assert(parse_position(NULL).row == -1);
    assert(parse_move_request_fields(NULL, "E4", PromotionChoice::None, &req) != Status::Ok);
    assert(parse_move_request_fields("E2", NULL, PromotionChoice::None, &req) != Status::Ok);
    assert(parse_move_request_fields("E2", "E4", (PromotionChoice)999, &req) != Status::Ok);

    /* Mutated strings fuzzing */
    static const char *const corpus[] = {"",
                                         " ",
                                         "\t\r\n",
                                         "A",
                                         "1",
                                         "A0",
                                         "A9",
                                         "K1",
                                         "J0",
                                         "J9",
                                         "E2E4",
                                         "E 2",
                                         "E2\0extra",
                                         "E2 ",
                                         " E2",
                                         "  E2  ",
                                         "A1",
                                         "J8",
                                         "a1",
                                         "j8",
                                         "e2",
                                         "e4",
                                         "%s%s%s%s%s%s%s%s%n",
                                         "\xFF\xFE\xFD",
                                         "棋子",
                                         "E\xFF"};

    for (size_t i = 0; i < sizeof(corpus) / sizeof(corpus[0]); ++i) {
        Square sq = parse_position(corpus[i]);
        (void)sq;
        for (size_t j = 0; j < sizeof(corpus) / sizeof(corpus[0]); ++j) {
            Status ret = parse_move_request_fields(corpus[i], corpus[j], PromotionChoice::None, &req);
            (void)ret;
        }
    }

    /* 10,000 iterations of random byte sequence fuzzing */
    for (int iter = 0; iter < 10000; ++iter) {
        int len = (int)(fuzz_rand(&rng) % 32);
        for (int b = 0; b < len; ++b) {
            buffer[b] = (char)(fuzz_rand(&rng) & 0xFF);
        }
        buffer[len] = '\0';

        Square sq = parse_position(buffer);
        if (is_valid_position(sq)) {
            assert(sq.row >= 0 && sq.row < Rows);
            assert(sq.col >= 0 && sq.col < Columns);
        }

        int promo = (int)(fuzz_rand(&rng) % 10) - 2;
        Status ret = parse_move_request_fields(buffer, buffer, (PromotionChoice)promo, &req);
        (void)ret;
    }
}

/* 2. Audit MoveList Capacity and Bound Invariants */
static void audit_movelist_bounds(void) {
    MoveList *list = static_cast<MoveList *>(malloc(sizeof(*list)));
    assert(list);

    init_move_list(list);
    assert(list->count == 0);
    assert(list->status == Status::Ok);

    Move dummy = create_move(create_position(1, 1), create_position(2, 1), create_piece(PieceType::Ant, Color::White));

    for (int i = 0; i < MaxMoves; ++i) {
        assert(add_move(list, dummy) == Status::Ok);
        assert(list->count == i + 1);
        assert(list->status == Status::Ok);
    }

    /* 1025th move must fail with Status::Capacity */
    assert(add_move(list, dummy) == Status::Capacity);
    assert(list->count == MaxMoves);
    assert(list->status == Status::Capacity);

    /* Stickiness: even after removing a move, additions must continue to fail until re-init */
    assert(remove_last_move(list) == Status::Ok);
    assert(list->count == MaxMoves - 1);
    assert(add_move(list, dummy) == Status::Capacity);
    assert(list->status == Status::Capacity);

    /* Re-init resets sticky status */
    init_move_list(list);
    assert(list->status == Status::Ok);
    assert(add_move(list, dummy) == Status::Ok);

    /* Out of bounds accessors */
    assert(get_move(list, -1) == NULL);
    assert(get_move(list, 0) != NULL);
    assert(get_move(list, 1) == NULL);
    assert(get_move(list, MaxMoves) == NULL);

    free(list);
}

#define TEST_AI_MIN_MOVE_BUDGET_MS 300
#define TEST_AI_TOURNAMENT_MAX_MS 10000
#define TEST_AI_TOURNAMENT_TOTAL_MS 600999

/* 3. Audit Time Budget and Tournament Arithmetic Boundaries */
static void audit_budget_arithmetic(void) {
    AITimeManager tm;
    init_ai_time_manager(&tm);

    /* Base tournament values */
    int budgetW = get_ai_tournament_budget_ms(&tm, Color::White);
    assert(budgetW >= TEST_AI_MIN_MOVE_BUDGET_MS && budgetW <= TEST_AI_TOURNAMENT_MAX_MS);

    /* Extreme elapsed time: larger than remaining */
    update_ai_tournament_time(&tm, Color::White, budgetW, 1000000);
    assert(tm.remainingMs[0] == 0);
    assert(is_ai_tournament_time_expired(&tm, Color::White));

    /* Check budget when expired */
    budgetW = get_ai_tournament_budget_ms(&tm, Color::White);
    assert(budgetW == TEST_AI_MIN_MOVE_BUDGET_MS);

    /* Check negative and boundary values */
    update_ai_tournament_time(&tm, Color::Black, -100, -500);
    assert(tm.remainingMs[1] == TEST_AI_TOURNAMENT_TOTAL_MS);

    /* GameConfig turn timer required seconds with extreme limit */
    GameConfig cfg;
    init_game_config_for_mode(&cfg, GameMode::HumanVsComputer);
    cfg.timerEnabled = 1;
    cfg.aiTimeLimit = INT_MAX / 1000 + 500;
    int budgetMs = get_ai_time_budget_ms(&cfg, Difficulty::Hard);
    assert(budgetMs == INT_MAX);

    int reqSec = get_required_ai_turn_timer_seconds(&cfg);
    assert(reqSec > 0);
}

/* 4. Audit Deep Random Playout: Hash Consistency & Apply/Unmake Invariants */
static void audit_random_playout_invariants(void) {
    uint32_t rng = 0x12345678;
    MoveList *moves = static_cast<MoveList *>(malloc(sizeof(*moves)));
    assert(moves);

    for (int game = 0; game < 20; ++game) {
        Position pos;
        position_init(&pos);

        Undo undoStack[200];
        int plyCount = 0;

        for (int ply = 0; ply < 150; ++ply) {
            assert(generate_legal_moves(&pos, moves) == Status::Ok);
            if (moves->count == 0) {
                break;
            }

            /* Pick random move */
            int choice = (int)(fuzz_rand(&rng) % (uint32_t)moves->count);
            Move move = moves->moves[choice];

            /* Validate move validation agreement */
            assert(validate_move(&pos, move));

            Undo undo;
            assert(position_apply(&pos, move, &undo) == Status::Ok);

            /* Crucial Invariant: Incremental XOR Hash MUST equal Full Recomputed Hash */
            assert(pos.hash == position_hash(&pos));

            undoStack[plyCount++] = undo;

            /* Check terminal result does not crash */
            GameResult res;
            assert(position_result(&pos, &res) == Status::Ok);
            if (res != GameResult::None) {
                break;
            }
        }

        /* Unmake all moves back to root and assert semantic equality */
        Position root;
        position_init(&root);

        for (int ply = plyCount - 1; ply >= 0; --ply) {
            assert(position_unmake(&pos, &undoStack[ply]) == Status::Ok);
            assert(pos.hash == position_hash(&pos));
        }

        assert((pos == root));
    }

    free(moves);
}

/* Injected clock for timeout/stale-result policy. Logs are a desktop concern. */
struct MockEnv {
    int64_t nowMs;
};
static int64_t mock_clock(void *p) {
    return static_cast<MockEnv *>(p)->nowMs;
}

/* 5. Audit Session Transactions and Stale Result Rejection */
static void audit_session_robustness(void) {
    MockEnv env{1000};
    auto owner = Session::create(SessionOptions{
        {mock_clock, &env}
    });
    Session *session = std::get_if<Session>(&owner);
    assert(session);
    GameConfig cfg;
    init_game_config_for_mode(&cfg, GameMode::HumanVsHuman);
    cfg.timerEnabled = 1;
    cfg.initialTimeSeconds = 10;
    assert(session->start(cfg) == Status::Ok);

    SessionState snap;
    snap = session->state();
    uint64_t rev = snap.revision;

    /* Move validation */
    MoveRequest req;
    assert(parse_move_request_fields("E2", "E4", PromotionChoice::None, &req) == Status::Ok);

    /* Simulate turn timeout: advance clock by 11 seconds */
    env.nowMs += 11000;

    /* Submit move on timed out position: must reject as stale because tick updated the turn */
    assert(session->submit(req) == Status::StaleResult);

    /* Verify session switched turn cleanly without corrupting position */
    snap = session->state();
    assert(snap.position.currentTurn == Color::Black);
    assert(snap.revision > rev);

    /* The next side can submit after the timeout. */
    assert(parse_move_request_fields("E7", "E5", PromotionChoice::None, &req) == Status::Ok);
    assert(session->submit(req) == Status::Ok);

    snap = session->state();
    assert(snap.historyCount == 1);
}

int main(void) {
    printf("[AUDIT] Running parser fuzzing...\n");
    audit_parser_fuzzing();

    printf("[AUDIT] Running MoveList capacity and boundary audit...\n");
    audit_movelist_bounds();

    printf("[AUDIT] Running budget arithmetic boundary audit...\n");
    audit_budget_arithmetic();

    printf("[AUDIT] Running 20-game random playout Hash and Undo audit...\n");
    audit_random_playout_invariants();

    printf("[AUDIT] Running Session transactional and stale rejection audit...\n");
    audit_session_robustness();

    printf("[AUDIT] All robustness checks PASSED.\n");
    return 0;
}
