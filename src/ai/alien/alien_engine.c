/*
 * Alien plugin — host-side bridge.
 *
 * This file lives in V_FINAL's world: it sees GameState / Move / Piece
 * via the core headers. It must NOT include alien_chess.h directly,
 * because alien's chess.h redefines Move/State/MoveList typedefs and
 * collides with V_FINAL's. The bridge talks to the alien runtime
 * exclusively through a flat C ABI implemented in alien_engine_native.c.
 */
#include "ai/alien/alien_engine.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/board.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "core/position.h"
#include "gameplay/movegen.h"

/* ---------- Flat ABI declared by alien_engine_native.c ---------- */

/* Alien piece-type encoding (see alien_chess.h: PAWN=1..ANTEATER=7,
 * negated for black). 0 = empty. */
enum {
    ALIEN_EMPTY    = 0,
    ALIEN_PAWN     = 1,
    ALIEN_KNIGHT   = 2,
    ALIEN_BISHOP   = 3,
    ALIEN_ROOK     = 4,
    ALIEN_QUEEN    = 5,
    ALIEN_KING     = 6,
    ALIEN_ANTEATER = 7
};

/* Color codes alien uses internally: WHITE=1, BLACK=2 */
enum { ALIEN_WHITE = 1, ALIEN_BLACK = 2 };

/* The alien-side worker. Implemented in alien_engine_native.c which
 * lives in alien's namespace and is the only file that includes
 * alien_chess.h. */
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
                              int *out_ant_eat);

/* ---------- V_FINAL → flat ABI translation ---------- */

static int v_final_piece_to_alien_code(Piece p) {
    int t;
    switch (p.type) {
    case ANT:      t = ALIEN_PAWN;     break;
    case KNIGHT:   t = ALIEN_KNIGHT;   break;
    case BISHOP:   t = ALIEN_BISHOP;   break;
    case ROOK:     t = ALIEN_ROOK;     break;
    case QUEEN:    t = ALIEN_QUEEN;    break;
    case KING:     t = ALIEN_KING;     break;
    case ANTEATER: t = ALIEN_ANTEATER; break;
    default:       return ALIEN_EMPTY;
    }
    return (p.color == WHITE) ? t : -t;
}

/* True if any move in history touched the given V_FINAL square (from/to/
 * captures/path). Mirrors alien's "moved" flags. */
static int square_touched(const GameState *s, int row, int col) {
    int i, j;
    for (i = 0; i < s->moveHistory.count; ++i) {
        const Move *m = &s->moveHistory.moves[i];
        if ((m->from.row == row && m->from.col == col) ||
            (m->to.row   == row && m->to.col   == col)) return 1;
        for (j = 0; j < m->captureCount; ++j) {
            if (m->captures[j].pos.row == row && m->captures[j].pos.col == col)
                return 1;
        }
        for (j = 0; j < m->pathLength; ++j) {
            if (m->path[j].row == row && m->path[j].col == col) return 1;
        }
    }
    return 0;
}

/* Alien rule: ep_col is the file of the pawn that just did a 2-step move,
 * regardless of whether any enemy ant is positioned to capture. */
static int alien_ep_col(const GameState *s) {
    if (s->moveHistory.count == 0) return -1;
    const Move *last = &s->moveHistory.moves[s->moveHistory.count - 1];
    if (last->movedPiece.type != ANT)   return -1;
    if (last->from.col != last->to.col) return -1;
    int delta = last->to.row - last->from.row;
    if (delta == 2 || delta == -2) return last->to.col;
    return -1;
}

/* Map alien's (special, ant_eat, castle, prom) flags to V_FINAL SpecialMove. */
static SpecialMove decode_alien_special(int prom, int castle, int ant_eat) {
    if (castle == 1) return CASTLING_KINGSIDE;
    if (castle == 2) return CASTLING_QUEENSIDE;
    if (prom == ALIEN_QUEEN)  return PROMOTION_QUEEN;
    if (prom == ALIEN_ROOK)   return PROMOTION_ROOK;
    if (prom == ALIEN_BISHOP) return PROMOTION_BISHOP;
    if (prom == ALIEN_KNIGHT) return PROMOTION_KNIGHT;
    if (ant_eat) return ANTEATER_CAPTURE;
    return NO_SPECIAL_MOVE;
}

/* Find the V_FINAL legal move that matches alien's (from, to, special).
 * Two-pass: exact, then from+to only (forgive minor special divergence). */
static int find_matching_legal_move(const GameState *state,
                                    int v_from_row, int v_from_col,
                                    int v_to_row,   int v_to_col,
                                    SpecialMove preferred,
                                    Move *out) {
    MoveList list;
    initMoveList(&list);
    if (generateLegalMoves(state, &list) != 0) return 1;

    for (int i = 0; i < list.count; ++i) {
        Move m = list.moves[i];
        if (m.from.row == v_from_row && m.from.col == v_from_col &&
            m.to.row   == v_to_row   && m.to.col   == v_to_col   &&
            m.specialType == preferred) {
            *out = m;
            return 0;
        }
    }
    for (int i = 0; i < list.count; ++i) {
        Move m = list.moves[i];
        if (m.from.row == v_from_row && m.from.col == v_from_col &&
            m.to.row   == v_to_row   && m.to.col   == v_to_col) {
            *out = m;
            return 0;
        }
    }
    return 1;   /* no match — caller falls back to own search */
}

/* ---------- Public entry ---------- */

int alien_plugin_generate_move(const GameState *state, Move *move, int time_ms) {
    int board[8][10];
    int castling[6];
    int wk_r = -1, wk_c = -1, bk_r = -1, bk_c = -1;
    int side;
    int ep_col;

    if (state == NULL || move == NULL) return 1;

    /* Translate the board with row-flip (V_FINAL row 0 = black home;
     * alien row 0 = white home). */
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            int alien_row = ROWS - 1 - r;
            board[alien_row][c] = v_final_piece_to_alien_code(state->board.cells[r][c]);
            Piece p = state->board.cells[r][c];
            if (p.type == KING) {
                if (p.color == WHITE) { wk_r = alien_row; wk_c = c; }
                else                  { bk_r = alien_row; bk_c = c; }
            }
        }
    }

    side = (state->currentTurn == WHITE) ? ALIEN_WHITE : ALIEN_BLACK;
    ep_col = alien_ep_col(state);

    /* Castling flags in alien indexing:
     *   [0]=wk_moved [1]=wra_moved [2]=wrj_moved
     *   [3]=bk_moved [4]=bra_moved [5]=brj_moved
     * Home squares in V_FINAL coords: (7,5)=WK, (7,0)=W A-rook, (7,9)=W J-rook,
     *                                  (0,5)=BK, (0,0)=B A-rook, (0,9)=B J-rook. */
    castling[0] = square_touched(state, 7, 5);
    castling[1] = square_touched(state, 7, 0);
    castling[2] = square_touched(state, 7, 9);
    castling[3] = square_touched(state, 0, 5);
    castling[4] = square_touched(state, 0, 0);
    castling[5] = square_touched(state, 0, 9);

    int fr, fc, tr, tc, chain_len, prom, castle, ant_eat;
    int chain_r[10], chain_c[10];
    if (alien_native_request_move(board, side, castling, ep_col,
                                  wk_r, wk_c, bk_r, bk_c,
                                  time_ms,
                                  &fr, &fc, &tr, &tc,
                                  &chain_len, chain_r, chain_c,
                                  &prom, &castle, &ant_eat) != 0) {
        return 1;
    }

    /* Translate alien (fr,fc,tr,tc) back to V_FINAL coords. */
    int v_from_row = ROWS - 1 - fr;
    int v_to_row   = ROWS - 1 - tr;
    SpecialMove sp = decode_alien_special(prom, castle, ant_eat);

    if (find_matching_legal_move(state,
                                 v_from_row, fc,
                                 v_to_row,   tc,
                                 sp, move) != 0) {
        return 1;
    }
    return 0;
}
