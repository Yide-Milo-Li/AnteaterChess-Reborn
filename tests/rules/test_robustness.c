#include "anteater/rules.h"
#include "anteater/session.h"
#include "anteater/ai.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Simple LCG pseudo-random number generator for reproducible fuzzing */
static uint32_t fuzz_rand(uint32_t *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

/* 1. Audit Coordinate and Move Request Parser Robustness against Malformed Inputs */
static void audit_parser_fuzzing(void) {
    uint32_t rng = 0xDEADBEEF;
    AcMoveRequest req;
    char buffer[128];

    /* Basic boundary inputs */
    assert(ac_parse_position(NULL).row == -1);
    assert(ac_parse_move_request_fields(NULL, "E4", AC_PROMOTION_CHOICE_NONE, &req) != 0);
    assert(ac_parse_move_request_fields("E2", NULL, AC_PROMOTION_CHOICE_NONE, &req) != 0);
    assert(ac_parse_move_request_fields("E2", "E4", (AcPromotionChoice)999, &req) != 0);

    /* Mutated strings fuzzing */
    static const char *const corpus[] = {
        "", " ", "\t\r\n", "A", "1", "A0", "A9", "K1", "J0", "J9",
        "E2E4", "E 2", "E2\0extra", "E2 ", " E2", "  E2  ",
        "A1", "J8", "a1", "j8", "e2", "e4",
        "%s%s%s%s%s%s%s%s%n", "\xFF\xFE\xFD", "棋子", "E\xFF"
    };

    for (size_t i = 0; i < sizeof(corpus) / sizeof(corpus[0]); ++i) {
        AcSquare sq = ac_parse_position(corpus[i]);
        (void)sq;
        for (size_t j = 0; j < sizeof(corpus) / sizeof(corpus[0]); ++j) {
            int ret = ac_parse_move_request_fields(corpus[i], corpus[j], AC_PROMOTION_CHOICE_NONE, &req);
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

        AcSquare sq = ac_parse_position(buffer);
        if (ac_is_valid_position(sq)) {
            assert(sq.row >= 0 && sq.row < AC_ROWS);
            assert(sq.col >= 0 && sq.col < AC_COLS);
        }

        int promo = (int)(fuzz_rand(&rng) % 10) - 2;
        int ret = ac_parse_move_request_fields(buffer, buffer, (AcPromotionChoice)promo, &req);
        (void)ret;
    }
}

/* 2. Audit MoveList Capacity and Bound Invariants */
static void audit_movelist_bounds(void) {
    AcMoveList *list = malloc(sizeof(*list));
    assert(list);

    ac_init_move_list(list);
    assert(list->count == 0);
    assert(list->status == AC_OK);

    AcMove dummy = ac_create_move(ac_create_position(1, 1), ac_create_position(2, 1),
                                  ac_create_piece(AC_ANT, AC_WHITE));

    for (int i = 0; i < AC_MAX_MOVES; ++i) {
        assert(ac_add_move(list, dummy) == AC_OK);
        assert(list->count == i + 1);
        assert(list->status == AC_OK);
    }

    /* 1025th move must fail with AC_CAPACITY */
    assert(ac_add_move(list, dummy) == AC_CAPACITY);
    assert(list->count == AC_MAX_MOVES);
    assert(list->status == AC_CAPACITY);

    /* Stickiness: even after removing a move, additions must continue to fail until re-init */
    assert(ac_remove_last_move(list) == AC_OK);
    assert(list->count == AC_MAX_MOVES - 1);
    assert(ac_add_move(list, dummy) == AC_CAPACITY);
    assert(list->status == AC_CAPACITY);

    /* Re-init resets sticky status */
    ac_init_move_list(list);
    assert(list->status == AC_OK);
    assert(ac_add_move(list, dummy) == AC_OK);

    /* Out of bounds accessors */
    assert(ac_get_move(list, -1) == NULL);
    assert(ac_get_move(list, 0) != NULL);
    assert(ac_get_move(list, 1) == NULL);
    assert(ac_get_move(list, AC_MAX_MOVES) == NULL);

    free(list);
}

#define TEST_AI_MIN_MOVE_BUDGET_MS 300
#define TEST_AI_TOURNAMENT_MAX_MS 10000
#define TEST_AI_TOURNAMENT_TOTAL_MS 600999

/* 3. Audit Time Budget and Tournament Arithmetic Boundaries */
static void audit_budget_arithmetic(void) {
    AcAITimeManager tm;
    ac_init_ai_time_manager(&tm);

    /* Base tournament values */
    int budgetW = ac_get_ai_tournament_budget_ms(&tm, AC_WHITE);
    assert(budgetW >= TEST_AI_MIN_MOVE_BUDGET_MS && budgetW <= TEST_AI_TOURNAMENT_MAX_MS);

    /* Extreme elapsed time: larger than remaining */
    ac_update_ai_tournament_time(&tm, AC_WHITE, budgetW, 1000000);
    assert(tm.remainingMs[0] == 0);
    assert(ac_is_ai_tournament_time_expired(&tm, AC_WHITE));

    /* Check budget when expired */
    budgetW = ac_get_ai_tournament_budget_ms(&tm, AC_WHITE);
    assert(budgetW == TEST_AI_MIN_MOVE_BUDGET_MS);

    /* Check negative and boundary values */
    ac_update_ai_tournament_time(&tm, AC_BLACK, -100, -500);
    assert(tm.remainingMs[1] == TEST_AI_TOURNAMENT_TOTAL_MS);

    /* GameConfig turn timer required seconds with extreme limit */
    AcGameConfig cfg;
    ac_init_game_config_for_mode(&cfg, AC_MODE_HUMAN_VS_COMPUTER);
    cfg.timerEnabled = 1;
    cfg.aiTimeLimit = INT_MAX / 1000 + 500;
    int budgetMs = ac_get_ai_time_budget_ms(&cfg, AC_DIFFICULTY_HARD);
    assert(budgetMs == INT_MAX);

    int reqSec = ac_get_required_ai_turn_timer_seconds(&cfg);
    assert(reqSec > 0);
}

/* 4. Audit Deep Random Playout: Hash Consistency & Apply/Unmake Invariants */
static void audit_random_playout_invariants(void) {
    uint32_t rng = 0x12345678;
    AcMoveList *moves = malloc(sizeof(*moves));
    assert(moves);

    for (int game = 0; game < 20; ++game) {
        AcPosition pos;
        ac_position_init(&pos);

        AcUndo undoStack[200];
        int plyCount = 0;

        for (int ply = 0; ply < 150; ++ply) {
            assert(ac_generate_legal_moves(&pos, moves) == AC_OK);
            if (moves->count == 0) {
                break;
            }

            /* Pick random move */
            int choice = (int)(fuzz_rand(&rng) % (uint32_t)moves->count);
            AcMove move = moves->moves[choice];

            /* Validate move validation agreement */
            assert(ac_validate_move(&pos, move));

            AcUndo undo;
            assert(ac_position_apply(&pos, move, &undo) == AC_OK);

            /* Crucial Invariant: Incremental XOR Hash MUST equal Full Recomputed Hash */
            assert(pos.hash == ac_position_hash(&pos));

            undoStack[plyCount++] = undo;

            /* Check terminal result does not crash */
            AcGameResult res;
            assert(ac_position_result(&pos, &res) == AC_OK);
            if (res != AC_RESULT_NONE) {
                break;
            }
        }

        /* Unmake all moves back to root and assert byte-exact match */
        AcPosition root;
        ac_position_init(&root);

        for (int ply = plyCount - 1; ply >= 0; --ply) {
            assert(ac_position_unmake(&pos, &undoStack[ply]) == AC_OK);
            assert(pos.hash == ac_position_hash(&pos));
        }

        assert(memcmp(&pos, &root, sizeof(AcPosition)) == 0);
    }

    free(moves);
}

/* Dummy clock and log for session test */
typedef struct {
    int64_t nowMs;
    int logFail;
    int logCount;
} MockEnv;

static int64_t mock_clock(void *ctx) {
    return ((MockEnv *)ctx)->nowMs;
}

static AcStatus mock_log(void *ctx, const AcSnapshot *snap) {
    (void)snap;
    MockEnv *env = ctx;
    ++env->logCount;
    return env->logFail ? AC_IO_ERROR : AC_OK;
}

/* 5. Audit Session Transactions and Stale Result Rejection */
static void audit_session_robustness(void) {
    MockEnv env = {.nowMs = 1000, .logFail = 0, .logCount = 0};
    AcSessionOptions opts = {
        .clock = {mock_clock, &env},
        .log = mock_log,
        .logContext = &env,
        .allocate = NULL,
        .deallocate = NULL,
        .allocatorContext = NULL
    };

    AcSession *session = ac_session_create(&opts);
    assert(session);

    AcGameConfig cfg;
    ac_init_game_config_for_mode(&cfg, AC_MODE_HUMAN_VS_HUMAN);
    cfg.timerEnabled = 1;
    cfg.initialTimeSeconds = 10;
    assert(ac_session_start(session, &cfg) == AC_OK);

    AcSnapshot snap;
    assert(ac_session_snapshot(session, &snap) == AC_OK);
    uint64_t rev = snap.revision;

    /* Move validation */
    AcMoveRequest req;
    assert(ac_parse_move_request_fields("E2", "E4", AC_PROMOTION_CHOICE_NONE, &req) == 0);

    /* Simulate turn timeout: advance clock by 11 seconds */
    env.nowMs += 11000;

    /* Submit move on timed out position: must reject as stale because tick updated the turn */
    assert(ac_session_submit(session, req) == AC_STALE_RESULT);

    /* Verify session switched turn cleanly without corrupting position */
    assert(ac_session_snapshot(session, &snap) == AC_OK);
    assert(snap.position.currentTurn == AC_BLACK);
    assert(snap.revision > rev);

    /* Verify diagnostic decoupling: log write failure does not roll back move */
    env.logFail = 1;
    assert(ac_parse_move_request_fields("E7", "E5", AC_PROMOTION_CHOICE_NONE, &req) == 0);
    assert(ac_session_submit(session, req) == AC_OK);

    assert(ac_session_snapshot(session, &snap) == AC_OK);
    assert(snap.historyCount == 1);
    assert(snap.diagnostic == AC_IO_ERROR); /* Diagnostic records failure */

    ac_session_destroy(session);
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
