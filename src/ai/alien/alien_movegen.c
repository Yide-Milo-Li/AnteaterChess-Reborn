#include "alien_namespace.h"
#include "alien_chess.h"

/* Direction arrays */
static const int knight_dr[] = {-2,-2,-1,-1, 1, 1, 2, 2};
static const int knight_dc[] = {-1, 1,-2, 2,-2, 2,-1, 1};
static const int king_dr[]   = {-1,-1,-1, 0, 0, 1, 1, 1};
static const int king_dc[]   = {-1, 0, 1,-1, 1,-1, 0, 1};
static const int rook_dr[]   = {-1, 0, 1, 0};
static const int rook_dc[]   = { 0, 1, 0,-1};
static const int bishop_dr[] = {-1,-1, 1, 1};
static const int bishop_dc[] = {-1, 1,-1, 1};

/* Helper: create a basic move */
static Move make_basic_move(int fr, int fc, int tr, int tc, int captured) {
    Move m;
    memset(&m, 0, sizeof(Move));
    m.fr = fr; m.fc = fc;
    m.tr = tr; m.tc = tc;
    m.captured = captured;
    return m;
}

/* Helper: add move to list */
static void add_move(MoveList *ml, Move m) {
    if (ml->count < MAX_MOVES)
        ml->moves[ml->count++] = m;
}

/* Generate pawn moves */
static void gen_pawn_moves(const State *s, int r, int c, MoveList *ml) {
    int color = PIECE_COLOR(s->board[r][c]);
    int dir = (color == WHITE) ? 1 : -1;
    int start_row = (color == WHITE) ? 1 : 6;
    int promo_row = (color == WHITE) ? 7 : 0;
    int ep_row = (color == WHITE) ? 4 : 3;
    int nr, nc;
    Move m;

    /* Forward one step */
    nr = r + dir;
    if (valid_pos(nr, c) && IS_EMPTY(s->board[nr][c])) {
        if (nr == promo_row) {
            /* Promotion */
            int promos[] = {QUEEN, ROOK, BISHOP, KNIGHT};
            int i;
            for (i = 0; i < 4; i++) {
                m = make_basic_move(r, c, nr, c, 0);
                m.promotion = promos[i];
                add_move(ml, m);
            }
        } else {
            add_move(ml, make_basic_move(r, c, nr, c, 0));

            /* Forward two steps from starting position */
            if (r == start_row) {
                int nr2 = r + 2 * dir;
                if (valid_pos(nr2, c) && IS_EMPTY(s->board[nr2][c])) {
                    add_move(ml, make_basic_move(r, c, nr2, c, 0));
                }
            }
        }
    }

    /* Captures (diagonal) */
    for (nc = c - 1; nc <= c + 1; nc += 2) {
        nr = r + dir;
        if (!valid_pos(nr, nc)) continue;
        int target = s->board[nr][nc];
        if (!IS_EMPTY(target) && PIECE_COLOR(target) != color) {
            /* Cannot capture with pawn if target is not capturable by pawn */
            /* Pawns capture normally (any enemy piece) */
            if (nr == promo_row) {
                int promos[] = {QUEEN, ROOK, BISHOP, KNIGHT};
                int i;
                for (i = 0; i < 4; i++) {
                    m = make_basic_move(r, c, nr, nc, target);
                    m.promotion = promos[i];
                    add_move(ml, m);
                }
            } else {
                add_move(ml, make_basic_move(r, c, nr, nc, target));
            }
        }

        /* En passant */
        if (r == ep_row && s->ep_col == nc) {
            int ep_pawn_row = r; /* the pawn to capture is on our row */
            int ep_pawn = s->board[ep_pawn_row][nc];
            if (!IS_EMPTY(ep_pawn) && PIECE_COLOR(ep_pawn) != color
                && PIECE_TYPE(ep_pawn) == PAWN) {
                m = make_basic_move(r, c, nr, nc, ep_pawn);
                m.en_passant = 1;
                add_move(ml, m);
            }
        }
    }
}

/* Generate knight moves */
static void gen_knight_moves(const State *s, int r, int c, MoveList *ml) {
    int color = PIECE_COLOR(s->board[r][c]);
    int i, nr, nc, target;

    for (i = 0; i < 8; i++) {
        nr = r + knight_dr[i];
        nc = c + knight_dc[i];
        if (!valid_pos(nr, nc)) continue;
        target = s->board[nr][nc];
        if (IS_EMPTY(target) || PIECE_COLOR(target) != color) {
            add_move(ml, make_basic_move(r, c, nr, nc, target));
        }
    }
}

/* Generate sliding piece moves (bishop, rook, queen) */
static void gen_sliding_moves(const State *s, int r, int c, MoveList *ml,
                              const int *dr, const int *dc, int ndirs) {
    int color = PIECE_COLOR(s->board[r][c]);
    int i, nr, nc, target;

    for (i = 0; i < ndirs; i++) {
        nr = r + dr[i];
        nc = c + dc[i];
        while (valid_pos(nr, nc)) {
            target = s->board[nr][nc];
            if (IS_EMPTY(target)) {
                add_move(ml, make_basic_move(r, c, nr, nc, 0));
            } else {
                if (PIECE_COLOR(target) != color) {
                    add_move(ml, make_basic_move(r, c, nr, nc, target));
                }
                break; /* blocked */
            }
            nr += dr[i];
            nc += dc[i];
        }
    }
}

/* Generate king moves (excluding castling) */
static void gen_king_moves(const State *s, int r, int c, MoveList *ml) {
    int color = PIECE_COLOR(s->board[r][c]);
    int i, nr, nc, target;

    for (i = 0; i < 8; i++) {
        nr = r + king_dr[i];
        nc = c + king_dc[i];
        if (!valid_pos(nr, nc)) continue;
        target = s->board[nr][nc];
        if (IS_EMPTY(target) || PIECE_COLOR(target) != color) {
            add_move(ml, make_basic_move(r, c, nr, nc, target));
        }
    }
}

/* Generate castling moves */
static void gen_castling(const State *s, int r, int c, MoveList *ml) {
    int color = PIECE_COLOR(s->board[r][c]);
    int row = (color == WHITE) ? 0 : 7;
    Move m;
    int i;

    if (r != row || c != 5) return; /* king not in original position */
    if (color == WHITE && s->wk_moved) return;
    if (color == BLACK && s->bk_moved) return;

    /* Can't castle while in check */
    if (is_attacked(s, r, 5, OPP_COLOR(color))) return;

    /* Kingside: king F -> H, rook J -> G */
    {
        int can = 1;
        if (color == WHITE && s->wrj_moved) can = 0;
        if (color == BLACK && s->brj_moved) can = 0;
        /* Check rook is there */
        if (can && PIECE_TYPE(s->board[row][9]) != ROOK) can = 0;
        if (can && PIECE_COLOR(s->board[row][9]) != color) can = 0;
        /* Squares between (cols 6,7,8) must be empty */
        if (can) {
            for (i = 6; i <= 8; i++) {
                if (!IS_EMPTY(s->board[row][i])) { can = 0; break; }
            }
        }
        /* King must not pass through or land on attacked squares (cols 6,7) */
        if (can && is_attacked(s, row, 6, OPP_COLOR(color))) can = 0;
        if (can && is_attacked(s, row, 7, OPP_COLOR(color))) can = 0;
        if (can) {
            m = make_basic_move(r, c, row, 7, 0);
            m.castle = 1;
            add_move(ml, m);
        }
    }

    /* Queenside: king F -> D, rook A -> E */
    {
        int can = 1;
        if (color == WHITE && s->wra_moved) can = 0;
        if (color == BLACK && s->bra_moved) can = 0;
        /* Check rook is there */
        if (can && PIECE_TYPE(s->board[row][0]) != ROOK) can = 0;
        if (can && PIECE_COLOR(s->board[row][0]) != color) can = 0;
        /* Squares between (cols 1,2,3,4) must be empty */
        if (can) {
            for (i = 1; i <= 4; i++) {
                if (!IS_EMPTY(s->board[row][i])) { can = 0; break; }
            }
        }
        /* King must not pass through or land on attacked squares (cols 4,3) */
        if (can && is_attacked(s, row, 4, OPP_COLOR(color))) can = 0;
        if (can && is_attacked(s, row, 3, OPP_COLOR(color))) can = 0;
        if (can) {
            m = make_basic_move(r, c, row, 3, 0);
            m.castle = 2;
            add_move(ml, m);
        }
    }
}

/*
 * Anteater ant-eating chain generation using DFS.
 * The anteater eats a pawn at the first position, then can continue
 * eating adjacent pawns along the same file or same rank.
 */
static void ant_eat_dfs(const State *s, int orig_r, int orig_c,
                        Move *m, int visited[ROWS][COLS],
                        MoveList *ml, int enemy_color) {
    /* Current position is the last in the chain */
    int cur_r = m->chain_r[m->chain_len - 1];
    int cur_c = m->chain_c[m->chain_len - 1];
    int dr, dc, nr, nc;

    /* This is already a valid move (stopping here) — add it */
    Move copy = *m;
    copy.tr = cur_r;
    copy.tc = cur_c;
    add_move(ml, copy);

    /* Try extending along same file (vertical neighbors) */
    for (dr = -1; dr <= 1; dr += 2) {
        nr = cur_r + dr;
        nc = cur_c;
        if (!valid_pos(nr, nc)) continue;
        if (visited[nr][nc]) continue;
        if (PIECE_TYPE(s->board[nr][nc]) == PAWN &&
            PIECE_COLOR(s->board[nr][nc]) == enemy_color) {
            visited[nr][nc] = 1;
            m->chain_r[m->chain_len] = nr;
            m->chain_c[m->chain_len] = nc;
            m->chain_cap[m->chain_len] = s->board[nr][nc];
            m->chain_len++;
            ant_eat_dfs(s, orig_r, orig_c, m, visited, ml, enemy_color);
            m->chain_len--;
            visited[nr][nc] = 0;
        }
    }

    /* Try extending along same rank (horizontal neighbors) */
    for (dc = -1; dc <= 1; dc += 2) {
        nr = cur_r;
        nc = cur_c + dc;
        if (!valid_pos(nr, nc)) continue;
        if (visited[nr][nc]) continue;
        if (PIECE_TYPE(s->board[nr][nc]) == PAWN &&
            PIECE_COLOR(s->board[nr][nc]) == enemy_color) {
            visited[nr][nc] = 1;
            m->chain_r[m->chain_len] = nr;
            m->chain_c[m->chain_len] = nc;
            m->chain_cap[m->chain_len] = s->board[nr][nc];
            m->chain_len++;
            ant_eat_dfs(s, orig_r, orig_c, m, visited, ml, enemy_color);
            m->chain_len--;
            visited[nr][nc] = 0;
        }
    }
}

/* Generate anteater moves */
static void gen_anteater_moves(const State *s, int r, int c, MoveList *ml) {
    int color = PIECE_COLOR(s->board[r][c]);
    int enemy = OPP_COLOR(color);
    int i, nr, nc, target;

    for (i = 0; i < 8; i++) {
        nr = r + king_dr[i];
        nc = c + king_dc[i];
        if (!valid_pos(nr, nc)) continue;
        target = s->board[nr][nc];

        if (IS_EMPTY(target)) {
            /* Simple move to empty square */
            add_move(ml, make_basic_move(r, c, nr, nc, 0));
        } else if (PIECE_TYPE(target) == PAWN && PIECE_COLOR(target) == enemy) {
            /* Start ant-eating chain */
            int visited[ROWS][COLS];
            Move m;
            memset(visited, 0, sizeof(visited));
            memset(&m, 0, sizeof(Move));
            m.fr = r; m.fc = c;
            m.ant_eating = 1;
            m.chain_r[0] = nr;
            m.chain_c[0] = nc;
            m.chain_cap[0] = target;
            m.chain_len = 1;
            visited[nr][nc] = 1;
            visited[r][c] = 1; /* don't revisit origin */
            ant_eat_dfs(s, r, c, &m, visited, ml, enemy);
        }
        /* Anteater cannot capture any other piece type */
    }
}

/*
 * Generate all pseudo-legal moves for the side to move.
 * "Pseudo-legal" = valid piece movement but might leave own king in check.
 */
void gen_pseudo_moves(const State *s, MoveList *ml) {
    int r, c, p, color, type;
    ml->count = 0;
    color = s->side;

    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            p = s->board[r][c];
            if (IS_EMPTY(p) || PIECE_COLOR(p) != color) continue;
            type = PIECE_TYPE(p);

            switch (type) {
                case PAWN:
                    gen_pawn_moves(s, r, c, ml);
                    break;
                case KNIGHT:
                    gen_knight_moves(s, r, c, ml);
                    break;
                case BISHOP:
                    gen_sliding_moves(s, r, c, ml, bishop_dr, bishop_dc, 4);
                    break;
                case ROOK:
                    gen_sliding_moves(s, r, c, ml, rook_dr, rook_dc, 4);
                    break;
                case QUEEN:
                    gen_sliding_moves(s, r, c, ml, bishop_dr, bishop_dc, 4);
                    gen_sliding_moves(s, r, c, ml, rook_dr, rook_dc, 4);
                    break;
                case KING:
                    gen_king_moves(s, r, c, ml);
                    gen_castling(s, r, c, ml);
                    break;
                case ANTEATER:
                    gen_anteater_moves(s, r, c, ml);
                    break;
            }
        }
    }
}

/*
 * Check if a square is attacked by a given color.
 * NOTE: Anteaters do NOT threaten squares (they can't check the king).
 */
int is_attacked(const State *s, int r, int c, int by_color) {
    int i, nr, nc, p;

    /* Check pawn attacks */
    {
        int pawn_dir = (by_color == WHITE) ? -1 : 1;
        /* A pawn of by_color on row r+pawn_dir attacks (r,c) diagonally */
        nr = r + pawn_dir;
        for (i = -1; i <= 1; i += 2) {
            nc = c + i;
            if (valid_pos(nr, nc)) {
                p = s->board[nr][nc];
                if (PIECE_TYPE(p) == PAWN && PIECE_COLOR(p) == by_color)
                    return 1;
            }
        }
    }

    /* Check knight attacks */
    for (i = 0; i < 8; i++) {
        nr = r + knight_dr[i];
        nc = c + knight_dc[i];
        if (valid_pos(nr, nc)) {
            p = s->board[nr][nc];
            if (PIECE_TYPE(p) == KNIGHT && PIECE_COLOR(p) == by_color)
                return 1;
        }
    }

    /* Check king attacks */
    for (i = 0; i < 8; i++) {
        nr = r + king_dr[i];
        nc = c + king_dc[i];
        if (valid_pos(nr, nc)) {
            p = s->board[nr][nc];
            if (PIECE_TYPE(p) == KING && PIECE_COLOR(p) == by_color)
                return 1;
        }
    }

    /* Check rook/queen attacks (straight lines) */
    for (i = 0; i < 4; i++) {
        nr = r + rook_dr[i];
        nc = c + rook_dc[i];
        while (valid_pos(nr, nc)) {
            p = s->board[nr][nc];
            if (!IS_EMPTY(p)) {
                if (PIECE_COLOR(p) == by_color &&
                    (PIECE_TYPE(p) == ROOK || PIECE_TYPE(p) == QUEEN))
                    return 1;
                break;
            }
            nr += rook_dr[i];
            nc += rook_dc[i];
        }
    }

    /* Check bishop/queen attacks (diagonals) */
    for (i = 0; i < 4; i++) {
        nr = r + bishop_dr[i];
        nc = c + bishop_dc[i];
        while (valid_pos(nr, nc)) {
            p = s->board[nr][nc];
            if (!IS_EMPTY(p)) {
                if (PIECE_COLOR(p) == by_color &&
                    (PIECE_TYPE(p) == BISHOP || PIECE_TYPE(p) == QUEEN))
                    return 1;
                break;
            }
            nr += bishop_dr[i];
            nc += bishop_dc[i];
        }
    }

    /* NOTE: Anteaters do NOT attack squares (no threat to king) */

    return 0;
}

/* Check if a color's king is in check */
int in_check(const State *s, int color) {
    int kr, kc;
    if (color == WHITE) {
        kr = s->wk_r; kc = s->wk_c;
    } else {
        kr = s->bk_r; kc = s->bk_c;
    }
    return is_attacked(s, kr, kc, OPP_COLOR(color));
}

/*
 * Apply a move to the state. Modifies state in place.
 * Caller should copy state beforehand if undo is needed.
 */
void make_move(State *s, const Move *m) {
    int piece = s->board[m->fr][m->fc];
    int color = PIECE_COLOR(piece);
    int type = PIECE_TYPE(piece);
    int i;

    /* Handle ant-eating chain */
    if (m->ant_eating) {
        s->board[m->fr][m->fc] = EMPTY;
        for (i = 0; i < m->chain_len; i++) {
            s->board[m->chain_r[i]][m->chain_c[i]] = EMPTY;
        }
        s->board[m->tr][m->tc] = piece;
        s->ep_col = -1;
        s->halfmove = 0; /* captures reset clock */
        goto update_flags;
    }

    /* Handle castling */
    if (m->castle) {
        int row = m->fr;
        s->board[row][5] = EMPTY; /* king leaves */

        if (m->castle == 1) {
            /* Kingside: king to col 7, rook from col 9 to col 6 */
            s->board[row][7] = piece;
            s->board[row][9] = EMPTY;
            s->board[row][6] = MAKE_PIECE(ROOK, color);
            if (color == WHITE) { s->wk_c = 7; }
            else { s->bk_c = 7; }
        } else {
            /* Queenside: king to col 3, rook from col 0 to col 4 */
            s->board[row][3] = piece;
            s->board[row][0] = EMPTY;
            s->board[row][4] = MAKE_PIECE(ROOK, color);
            if (color == WHITE) { s->wk_c = 3; }
            else { s->bk_c = 3; }
        }

        if (color == WHITE) { s->wk_moved = 1; s->wra_moved = 1; s->wrj_moved = 1; }
        else { s->bk_moved = 1; s->bra_moved = 1; s->brj_moved = 1; }

        s->ep_col = -1;
        s->halfmove++;
        goto switch_side;
    }

    /* Handle en passant */
    if (m->en_passant) {
        s->board[m->fr][m->fc] = EMPTY;
        s->board[m->tr][m->tc] = piece;
        /* Remove the captured pawn (on same row as moving pawn) */
        s->board[m->fr][m->tc] = EMPTY;
        s->ep_col = -1;
        s->halfmove = 0;
        goto update_flags;
    }

    /* Regular move */
    s->board[m->fr][m->fc] = EMPTY;
    s->board[m->tr][m->tc] = piece;

    /* Handle promotion */
    if (m->promotion) {
        s->board[m->tr][m->tc] = MAKE_PIECE(m->promotion, color);
    }

    /* Update halfmove clock */
    if (type == PAWN || m->captured != 0) {
        s->halfmove = 0;
    } else {
        s->halfmove++;
    }

    /* Set en passant column if pawn double-moved */
    if (type == PAWN && abs(m->tr - m->fr) == 2) {
        s->ep_col = m->fc;
    } else {
        s->ep_col = -1;
    }

update_flags:
    /* Update king position */
    if (type == KING) {
        if (color == WHITE) {
            s->wk_r = m->tr; s->wk_c = m->tc;
            s->wk_moved = 1;
        } else {
            s->bk_r = m->tr; s->bk_c = m->tc;
            s->bk_moved = 1;
        }
    }

    /* Update castling rights if rook moved or captured */
    if (m->fr == 0 && m->fc == 0) s->wra_moved = 1;
    if (m->fr == 0 && m->fc == 9) s->wrj_moved = 1;
    if (m->fr == 7 && m->fc == 0) s->bra_moved = 1;
    if (m->fr == 7 && m->fc == 9) s->brj_moved = 1;
    if (m->tr == 0 && m->tc == 0) s->wra_moved = 1;
    if (m->tr == 0 && m->tc == 9) s->wrj_moved = 1;
    if (m->tr == 7 && m->tc == 0) s->bra_moved = 1;
    if (m->tr == 7 && m->tc == 9) s->brj_moved = 1;

switch_side:
    /* Switch side */
    s->side = OPP_COLOR(s->side);
    if (s->side == WHITE) s->move_num++;

    /* Recompute hash */
    s->hash = compute_hash(s);
}

/*
 * Generate all legal moves (pseudo-legal filtered for king safety).
 */
void gen_legal_moves(const State *s, MoveList *ml) {
    MoveList pseudo;
    int i;
    State tmp;

    gen_pseudo_moves(s, &pseudo);
    ml->count = 0;

    for (i = 0; i < pseudo.count; i++) {
        copy_state(&tmp, s);
        make_move(&tmp, &pseudo.moves[i]);

        /* After the move, check if OUR king is in check
         * (the side that just moved is now tmp.side's opponent) */
        if (!in_check(&tmp, s->side)) {
            add_move(ml, pseudo.moves[i]);
        }
    }
}

/* Check if the current side to move is in checkmate */
int is_checkmate(const State *s) {
    MoveList ml;
    if (!in_check(s, s->side)) return 0;
    gen_legal_moves(s, &ml);
    return ml.count == 0;
}

/* Check if the current side to move is in stalemate */
int is_stalemate(const State *s) {
    MoveList ml;
    if (in_check(s, s->side)) return 0;
    gen_legal_moves(s, &ml);
    return ml.count == 0;
}

/* Validate if a move matches any legal move and return the full legal move */
int is_legal_move(const State *s, const Move *m) {
    MoveList ml;
    int i;
    gen_legal_moves(s, &ml);
    for (i = 0; i < ml.count; i++) {
        Move *lm = &ml.moves[i];
        if (lm->fr == m->fr && lm->fc == m->fc &&
            lm->tr == m->tr && lm->tc == m->tc) {
            if (m->promotion && lm->promotion != m->promotion) continue;
            if (m->ant_eating && lm->ant_eating) {
                /* For ant eating, check chain matches */
                if (m->chain_len != lm->chain_len) continue;
                int match = 1, j;
                for (j = 0; j < m->chain_len; j++) {
                    if (m->chain_r[j] != lm->chain_r[j] ||
                        m->chain_c[j] != lm->chain_c[j]) {
                        match = 0; break;
                    }
                }
                if (!match) continue;
            }
            return 1;
        }
    }
    return 0;
}
