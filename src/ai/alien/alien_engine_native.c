/*
 * Alien plugin — native-side worker.
 *
 * This is the ONLY plugin file that lives in alien's namespace
 * (alien_chess.h types: Move, State, MoveList, ...). It exposes a single
 * C function `alien_native_request_move` that takes flat ints and writes
 * flat ints, decoupling the bridge from alien's typedef names.
 *
 * The bridge calls this via an `extern int alien_native_request_move(...)`
 * forward declaration in alien_engine.c — no header is shared.
 */
#include "alien_namespace.h"
#include "alien_chess.h"
#include "alien_book.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Functions provided by other alien_*.c files (linked into the same plugin). */
extern void init_zobrist(void);
extern void ai_init(void);
extern Move ai_best_move_ex(State *s, int time_ms, int max_depth);
extern uint64_t compute_hash(const State *s);
extern int  book_load_global(const char *filename);
extern void book_set_mode(int mode);

/*
 * The alien CLI defines print_move / print_move_str in its ui.c. We do
 * NOT ship that file here — alien is being driven from the V_FINAL host
 * which has its own renderer. Provide silent stubs so alien_ai.c and
 * alien_book.c link without needing the interactive frontend.
 */
void print_move(const Move *m) { (void)m; }
void print_move_str(const Move *m, char *buf, int bufsz) {
    (void)m;
    if (buf && bufsz > 0) buf[0] = '\0';
}

static int g_alien_inited = 0;

static void try_load_book_lazy(void) {
    /* Try a handful of paths so the engine works whether you launch from
     * the project root or from bin/. The book file ships next to this
     * source under src/ai/alien/opening.book. */
    const char *env = getenv("V_FINAL_ALIEN_BOOK");
    const char *candidates[] = {
        env,
        "src/ai/alien/opening.book",
        "../src/ai/alien/opening.book",
        "../../src/ai/alien/opening.book",
        "ai/alien/opening.book",
        "opening.book"
    };
    size_t n = sizeof(candidates) / sizeof(candidates[0]);
    for (size_t i = 0; i < n; ++i) {
        if (!candidates[i] || candidates[i][0] == '\0') continue;
        if (book_load_global(candidates[i])) {
            book_set_mode(1);   /* BOOK_MODE_CPU_ONLY */
            return;
        }
    }
    /* No book — alien still plays from search alone. */
    book_set_mode(0);           /* BOOK_MODE_DISABLED */
}

static void alien_lazy_init(void) {
    if (g_alien_inited) return;
    init_zobrist();
    ai_init();
    try_load_book_lazy();
    g_alien_inited = 1;
}

int alien_native_request_move(const int board[8][10],
                              int side,
                              const int castling[6],
                              int ep_col,
                              int wk_r, int wk_c,
                              int bk_r, int bk_c,
                              int time_ms,
                              int *out_fr, int *out_fc,
                              int *out_tr, int *out_tc,
                              int *out_chain_len,
                              int out_chain_r[10], int out_chain_c[10],
                              int *out_promotion,
                              int *out_castle,
                              int *out_ant_eat) {
    State s;
    Move m;
    int r, c, i;

    alien_lazy_init();

    /* Build the AlienState from the flat ABI inputs. */
    memset(&s, 0, sizeof(s));
    for (r = 0; r < ROWS; ++r)
        for (c = 0; c < COLS; ++c)
            s.board[r][c] = board[r][c];
    s.side       = side;
    s.wk_moved   = castling[0];
    s.wra_moved  = castling[1];
    s.wrj_moved  = castling[2];
    s.bk_moved   = castling[3];
    s.bra_moved  = castling[4];
    s.brj_moved  = castling[5];
    s.ep_col     = ep_col;
    s.wk_r = wk_r; s.wk_c = wk_c;
    s.bk_r = bk_r; s.bk_c = bk_c;
    s.move_num = 1;
    s.halfmove = 0;
    s.hash = compute_hash(&s);

    /* Run the alien search. time_ms 0 = unlimited; max_depth 0 = MAX_DEPTH. */
    m = ai_best_move_ex(&s, time_ms, 0);

    /* Detect "no move" (empty struct returned on no-legal-moves). */
    if (m.fr == 0 && m.fc == 0 && m.tr == 0 && m.tc == 0
        && m.chain_len == 0 && m.castle == 0 && m.promotion == 0
        && m.ant_eating == 0) {
        return 1;
    }

    *out_fr = m.fr; *out_fc = m.fc; *out_tr = m.tr; *out_tc = m.tc;
    *out_chain_len = m.chain_len;
    for (i = 0; i < m.chain_len && i < 10; ++i) {
        out_chain_r[i] = m.chain_r[i];
        out_chain_c[i] = m.chain_c[i];
    }
    *out_promotion = m.promotion;
    *out_castle    = m.castle;
    *out_ant_eat   = m.ant_eating;
    return 0;
}
