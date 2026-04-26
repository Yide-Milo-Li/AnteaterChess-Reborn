#include "alien_namespace.h"
#include "alien_chess.h"

/* ---- Piece Values (centipawns) ---- */
static const int piece_val[8] = {
    0,      /* EMPTY */
    100,    /* PAWN */
    320,    /* KNIGHT */
    330,    /* BISHOP */
    500,    /* ROOK */
    900,    /* QUEEN */
    20000,  /* KING */
    200     /* ANTEATER */
};

/* ---- Piece-Square Tables (from WHITE's perspective, row 0 = rank 1) ---- */
/* Adapted for 10-column board. Center columns (D-G = 3-6) are strong. */

static const int pst_pawn[ROWS][COLS] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  0,  0,  5,  5,  5,  5,  0,  0,  0},
    {  5,  5, 10, 15, 20, 20, 15, 10,  5,  5},
    {  5, 10, 15, 25, 30, 30, 25, 15, 10,  5},
    { 10, 15, 25, 35, 40, 40, 35, 25, 15, 10},
    { 20, 25, 35, 45, 50, 50, 45, 35, 25, 20},
    { 50, 50, 50, 50, 50, 50, 50, 50, 50, 50},
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};

static const int pst_knight[ROWS][COLS] = {
    {-50,-40,-30,-30,-30,-30,-30,-30,-40,-50},
    {-40,-20,  0,  5, 10, 10,  5,  0,-20,-40},
    {-30,  5, 15, 20, 20, 20, 20, 15,  5,-30},
    {-30,  5, 15, 25, 30, 30, 25, 15,  5,-30},
    {-30,  5, 15, 25, 30, 30, 25, 15,  5,-30},
    {-30,  5, 15, 20, 20, 20, 20, 15,  5,-30},
    {-40,-20,  0,  5, 10, 10,  5,  0,-20,-40},
    {-50,-40,-30,-30,-30,-30,-30,-30,-40,-50}
};

static const int pst_bishop[ROWS][COLS] = {
    {-20,-10,-10,-10,-10,-10,-10,-10,-10,-20},
    {-10, 10,  0,  0,  5,  5,  0,  0, 10,-10},
    {-10, 10, 10, 10, 10, 10, 10, 10, 10,-10},
    {-10,  0, 15, 20, 20, 20, 20, 15,  0,-10},
    {-10,  5, 15, 20, 20, 20, 20, 15,  5,-10},
    {-10,  0, 10, 15, 15, 15, 15, 10,  0,-10},
    {-10,  5,  0,  0,  0,  0,  0,  0,  5,-10},
    {-20,-10,-10,-10,-10,-10,-10,-10,-10,-20}
};

static const int pst_rook[ROWS][COLS] = {
    {  0,  0,  5, 10, 10, 10, 10,  5,  0,  0},
    { -5,  0,  0,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0,  0,  0, -5},
    { -5,  0,  0,  0,  0,  0,  0,  0,  0, -5},
    { 10, 15, 15, 15, 15, 15, 15, 15, 15, 10},
    {  0,  0,  5, 10, 10, 10, 10,  5,  0,  0}
};

static const int pst_queen[ROWS][COLS] = {
    {-20,-10,-10, -5, -5, -5, -5,-10,-10,-20},
    {-10,  0,  5,  0,  0,  0,  0,  5,  0,-10},
    {-10,  5,  5,  5,  5,  5,  5,  5,  5,-10},
    { -5,  0,  5, 10, 10, 10, 10,  5,  0, -5},
    { -5,  0,  5, 10, 10, 10, 10,  5,  0, -5},
    {-10,  0,  5,  5,  5,  5,  5,  5,  0,-10},
    {-10,  0,  0,  0,  0,  0,  0,  0,  0,-10},
    {-20,-10,-10, -5, -5, -5, -5,-10,-10,-20}
};

static const int pst_king_mid[ROWS][COLS] = {
    { 20, 30, 10,  0,  0,  0,  0, 10, 30, 20},
    { 20, 20,  0, -5, -5, -5, -5,  0, 20, 20},
    {-10,-20,-20,-20,-20,-20,-20,-20,-20,-10},
    {-20,-30,-30,-40,-40,-40,-40,-30,-30,-20},
    {-30,-40,-40,-50,-50,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-50,-50,-40,-40,-30},
    {-30,-40,-40,-50,-50,-50,-50,-40,-40,-30}
};

static const int pst_king_end[ROWS][COLS] = {
    {-50,-30,-30,-30,-30,-30,-30,-30,-30,-50},
    {-30,-10,  0, 10, 10, 10, 10,  0,-10,-30},
    {-30,  0, 20, 30, 30, 30, 30, 20,  0,-30},
    {-30,  0, 30, 40, 50, 50, 40, 30,  0,-30},
    {-30,  0, 30, 40, 50, 50, 40, 30,  0,-30},
    {-30,  0, 20, 30, 30, 30, 30, 20,  0,-30},
    {-30,-10,  0, 10, 10, 10, 10,  0,-10,-30},
    {-50,-30,-30,-30,-30,-30,-30,-30,-30,-50}
};

static const int pst_anteater[ROWS][COLS] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
    {  0,  5,  5,  5,  5,  5,  5,  5,  5,  0},
    {  5,  5, 10, 15, 15, 15, 15, 10,  5,  5},
    { 10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    { 10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {  5,  5, 10, 15, 15, 15, 15, 10,  5,  5},
    {  0,  5,  5,  5,  5,  5,  5,  5,  5,  0},
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};

/* ---- Transposition Table ---- */
static TTEntry tt[TT_SIZE];

/* ---- Search State ---- */
static clock_t search_start;
static int search_time_limit;
static int search_stopped;
static int nodes_searched;

/* Killer moves and history heuristic for move ordering */
static Move killers[MAX_DEPTH][2];
static int history[2][ROWS][COLS][ROWS][COLS]; /* [color_idx][from_r][from_c][to_r][to_c] */

void ai_init(void) {
    memset(tt, 0, sizeof(tt));
    memset(killers, 0, sizeof(killers));
    memset(history, 0, sizeof(history));
}

/* ---- Evaluation ---- */

static int count_material(const State *s, int color) {
    int r, c, total = 0;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            if (PIECE_COLOR(s->board[r][c]) == color)
                total += piece_val[PIECE_TYPE(s->board[r][c])];
    return total;
}

static int is_endgame(const State *s) {
    int mat = count_material(s, WHITE) + count_material(s, BLACK);
    return mat < 44000; /* roughly: both sides lost a queen worth of material */
}

/* Evaluate pawn structure */
static int eval_pawns(const State *s, int color) {
    int score = 0;
    int r, c;
    int dir = (color == WHITE) ? 1 : -1;
    int pawn_piece = MAKE_PIECE(PAWN, color);
    int enemy = OPP_COLOR(color);

    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            if (s->board[r][c] != pawn_piece) continue;

            /* Doubled pawns penalty */
            int rr;
            for (rr = r + dir; rr >= 0 && rr < ROWS; rr += dir) {
                if (s->board[rr][c] == pawn_piece) {
                    score -= 15;
                    break;
                }
            }

            /* Isolated pawn penalty */
            {
                int has_neighbor = 0;
                int cc;
                for (cc = c - 1; cc <= c + 1; cc += 2) {
                    if (cc < 0 || cc >= COLS) continue;
                    for (rr = 0; rr < ROWS; rr++) {
                        if (s->board[rr][cc] == pawn_piece) {
                            has_neighbor = 1;
                            break;
                        }
                    }
                    if (has_neighbor) break;
                }
                if (!has_neighbor) score -= 10;
            }

            /* Passed pawn bonus */
            {
                int passed = 1;
                int start_r = (color == WHITE) ? r + 1 : 0;
                int end_r = (color == WHITE) ? ROWS : r;
                for (rr = start_r; rr < end_r && passed; rr++) {
                    for (int cc = c - 1; cc <= c + 1; cc++) {
                        if (cc < 0 || cc >= COLS) continue;
                        if (s->board[rr][cc] == MAKE_PIECE(PAWN, enemy)) {
                            passed = 0;
                            break;
                        }
                    }
                }
                if (passed) {
                    int advance = (color == WHITE) ? r : (ROWS - 1 - r);
                    score += 10 + advance * 10;
                }
            }
        }
    }
    return score;
}

/* King safety: count pawns near king */
static int eval_king_safety(const State *s, int color) {
    int kr = (color == WHITE) ? s->wk_r : s->bk_r;
    int kc = (color == WHITE) ? s->wk_c : s->bk_c;
    int score = 0;
    int r, c;
    int pawn_piece = MAKE_PIECE(PAWN, color);

    /* Pawn shield bonus */
    int shield_dir = (color == WHITE) ? 1 : -1;
    for (c = kc - 1; c <= kc + 1; c++) {
        if (c < 0 || c >= COLS) continue;
        r = kr + shield_dir;
        if (r >= 0 && r < ROWS && s->board[r][c] == pawn_piece) {
            score += 10;
        }
        r = kr + 2 * shield_dir;
        if (r >= 0 && r < ROWS && s->board[r][c] == pawn_piece) {
            score += 5;
        }
    }

    /* Penalty for open files near king */
    for (c = kc - 1; c <= kc + 1; c++) {
        if (c < 0 || c >= COLS) continue;
        int has_own_pawn = 0;
        for (r = 0; r < ROWS; r++) {
            if (s->board[r][c] == pawn_piece) {
                has_own_pawn = 1;
                break;
            }
        }
        if (!has_own_pawn) score -= 15;
    }

    return score;
}

int evaluate(const State *s) {
    int score = 0;
    int r, c, p, type, color;
    int endgame = is_endgame(s);

    /* Material + piece-square tables */
    for (r = 0; r < ROWS; r++) {
        for (c = 0; c < COLS; c++) {
            p = s->board[r][c];
            if (IS_EMPTY(p)) continue;

            type = PIECE_TYPE(p);
            color = PIECE_COLOR(p);
            int val = piece_val[type];
            int pst = 0;

            int pr = (color == WHITE) ? r : (ROWS - 1 - r);

            switch (type) {
                case PAWN:     pst = pst_pawn[pr][c]; break;
                case KNIGHT:   pst = pst_knight[pr][c]; break;
                case BISHOP:   pst = pst_bishop[pr][c]; break;
                case ROOK:     pst = pst_rook[pr][c]; break;
                case QUEEN:    pst = pst_queen[pr][c]; break;
                case KING:
                    pst = endgame ? pst_king_end[pr][c] : pst_king_mid[pr][c];
                    break;
                case ANTEATER: pst = pst_anteater[pr][c]; break;
            }

            if (color == WHITE)
                score += val + pst;
            else
                score -= val + pst;
        }
    }

    /* Bishop pair bonus */
    {
        int wb = 0, bb = 0;
        for (r = 0; r < ROWS; r++)
            for (c = 0; c < COLS; c++) {
                if (s->board[r][c] == MAKE_PIECE(BISHOP, WHITE)) wb++;
                if (s->board[r][c] == MAKE_PIECE(BISHOP, BLACK)) bb++;
            }
        if (wb >= 2) score += 40;
        if (bb >= 2) score -= 40;
    }

    /* Pawn structure */
    score += eval_pawns(s, WHITE) - eval_pawns(s, BLACK);

    /* King safety (only in middlegame) */
    if (!endgame) {
        score += eval_king_safety(s, WHITE) - eval_king_safety(s, BLACK);
    }

    /* Castling rights bonus */
    if (!s->wk_moved) score += 15;
    if (!s->bk_moved) score -= 15;

    return (s->side == WHITE) ? score : -score;
}

/* ---- Move Ordering ---- */

static int mvv_lva_score(const Move *m, int piece_at_from) {
    if (m->ant_eating) {
        return 100 * m->chain_len + 10000;
    }
    if (m->captured == 0) return 0;
    int victim_val = piece_val[PIECE_TYPE(m->captured)];
    int attacker_val = piece_val[PIECE_TYPE(piece_at_from)];
    return victim_val * 10 - attacker_val + 10000;
}

static int move_eq(const Move *a, const Move *b) {
    return a->fr == b->fr && a->fc == b->fc &&
           a->tr == b->tr && a->tc == b->tc;
}

static void order_moves(const State *s, MoveList *ml,
                        const Move *hash_move, int depth) {
    int i;
    int side_idx = (s->side == WHITE) ? 0 : 1;

    for (i = 0; i < ml->count; i++) {
        Move *m = &ml->moves[i];
        m->score = 0;

        if (hash_move && move_eq(m, hash_move)) {
            m->score = 200000;
            continue;
        }

        /* Winning/equal captures by MVV-LVA */
        if (m->captured || m->ant_eating) {
            m->score = mvv_lva_score(m, s->board[m->fr][m->fc]) + 100000;
            continue;
        }

        /* Queen promotions */
        if (m->promotion == QUEEN) {
            m->score = 95000;
            continue;
        }
        if (m->promotion) {
            m->score = 90000;
            continue;
        }

        /* Killer moves */
        if (depth >= 0 && depth < MAX_DEPTH) {
            if (move_eq(m, &killers[depth][0])) { m->score = 80000; continue; }
            if (move_eq(m, &killers[depth][1])) { m->score = 70000; continue; }
        }

        /* History heuristic */
        m->score = history[side_idx][m->fr][m->fc][m->tr][m->tc];
    }

    /* Insertion sort */
    for (i = 1; i < ml->count; i++) {
        Move key = ml->moves[i];
        int j = i - 1;
        while (j >= 0 && ml->moves[j].score < key.score) {
            ml->moves[j + 1] = ml->moves[j];
            j--;
        }
        ml->moves[j + 1] = key;
    }
}

/* ---- Time Check ---- */
static int check_time(void) {
    if ((nodes_searched & 2047) == 0) {
        clock_t now = clock();
        int elapsed = (int)((now - search_start) * 1000 / CLOCKS_PER_SEC);
        if (elapsed >= search_time_limit) {
            search_stopped = 1;
            return 1;
        }
    }
    return search_stopped;
}

/* ---- Quiescence Search ---- */
static int quiescence(State *s, int alpha, int beta, int qs_depth) {
    if (check_time()) return 0;
    nodes_searched++;

    int stand_pat = evaluate(s);
    if (stand_pat >= beta) return beta;
    if (stand_pat > alpha) alpha = stand_pat;

    /* Limit quiescence depth */
    if (qs_depth >= 8) return alpha;

    /* Delta pruning: if no capture can raise alpha, skip */
    int big_delta = 1000; /* queen value + margin */
    if (stand_pat + big_delta < alpha) return alpha;

    MoveList ml;
    gen_pseudo_moves(s, &ml);

    /* Score and sort captures */
    int i;
    for (i = 0; i < ml.count; i++) {
        Move *m = &ml.moves[i];
        if (m->captured == 0 && !m->ant_eating && m->promotion != QUEEN)
            m->score = -1;
        else
            m->score = mvv_lva_score(m, s->board[m->fr][m->fc]);
    }
    /* Partial sort: pick best capture each iteration */
    for (i = 0; i < ml.count; i++) {
        /* Find best remaining */
        int best_idx = i;
        int j;
        for (j = i + 1; j < ml.count; j++) {
            if (ml.moves[j].score > ml.moves[best_idx].score)
                best_idx = j;
        }
        if (ml.moves[best_idx].score < 0) break; /* no more captures */
        if (best_idx != i) {
            Move tmp = ml.moves[i];
            ml.moves[i] = ml.moves[best_idx];
            ml.moves[best_idx] = tmp;
        }

        Move *m = &ml.moves[i];

        State tmp;
        copy_state(&tmp, s);
        make_move(&tmp, m);
        if (in_check(&tmp, s->side)) continue;

        int score = -quiescence(&tmp, -beta, -alpha, qs_depth + 1);
        if (search_stopped) return 0;

        if (score >= beta) return beta;
        if (score > alpha) alpha = score;
    }

    return alpha;
}

/* ---- Alpha-Beta with PVS ---- */
static int alpha_beta(State *s, int depth, int alpha, int beta, int do_null) {
    if (check_time()) return 0;

    int is_pv = (beta - alpha > 1);

    /* Transposition table lookup */
    int tt_idx = (int)(s->hash & (TT_SIZE - 1));
    TTEntry *tte = &tt[tt_idx];
    Move *hash_move = NULL;

    if (tte->valid && tte->hash == s->hash) {
        if (tte->depth >= depth && !is_pv) {
            if (tte->flag == TT_EXACT) return tte->score;
            if (tte->flag == TT_ALPHA && tte->score <= alpha) return alpha;
            if (tte->flag == TT_BETA && tte->score >= beta) return beta;
        }
        hash_move = &tte->best;
    }

    /* Leaf node -> quiescence */
    if (depth <= 0) {
        return quiescence(s, alpha, beta, 0);
    }

    nodes_searched++;
    int in_check_now = in_check(s, s->side);

    /* Check extension: search 1 ply deeper when in check */
    if (in_check_now) depth++;

    /* 50-move rule */
    if (s->halfmove >= 100) return 0;

    /* Null move pruning */
    if (do_null && !is_pv && depth >= 3 && !in_check_now) {
        State tmp;
        copy_state(&tmp, s);
        tmp.side = OPP_COLOR(tmp.side);
        tmp.ep_col = -1;
        tmp.hash = compute_hash(&tmp);

        int R = (depth >= 6) ? 3 : 2;
        int score = -alpha_beta(&tmp, depth - 1 - R, -beta, -beta + 1, 0);
        if (search_stopped) return 0;
        if (score >= beta) return beta;
    }

    /* Generate pseudo-legal moves and order them */
    MoveList ml;
    gen_pseudo_moves(s, &ml);
    order_moves(s, &ml, hash_move, depth);

    int legal_count = 0;
    int best_score = -INF;
    Move best_move;
    memset(&best_move, 0, sizeof(Move));
    int flag = TT_ALPHA;
    int side_idx = (s->side == WHITE) ? 0 : 1;

    int i;
    for (i = 0; i < ml.count; i++) {
        State tmp;
        copy_state(&tmp, s);
        make_move(&tmp, &ml.moves[i]);

        /* Check legality: own king must not be in check */
        if (in_check(&tmp, s->side)) continue;

        legal_count++;
        int score;

        if (legal_count == 1) {
            /* First move: full window search */
            score = -alpha_beta(&tmp, depth - 1, -beta, -alpha, 1);
        } else {
            /* Late Move Reduction */
            int reduction = 0;
            if (depth >= 3 && legal_count >= 4 &&
                !ml.moves[i].captured && !ml.moves[i].ant_eating &&
                !ml.moves[i].promotion && !in_check_now) {
                reduction = 1;
                if (legal_count >= 8) reduction = 2;
                if (depth <= 4) reduction = 1;
            }

            /* PVS: null window search */
            score = -alpha_beta(&tmp, depth - 1 - reduction,
                                -alpha - 1, -alpha, 1);

            /* Re-search at full depth/window if needed */
            if (score > alpha && (reduction > 0 || score < beta)) {
                copy_state(&tmp, s);
                make_move(&tmp, &ml.moves[i]);
                score = -alpha_beta(&tmp, depth - 1, -beta, -alpha, 1);
            }
        }

        if (search_stopped) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = ml.moves[i];
        }

        if (score > alpha) {
            alpha = score;
            flag = TT_EXACT;

            /* Update history for quiet moves that improve alpha */
            if (!ml.moves[i].captured && !ml.moves[i].ant_eating) {
                {
                    int *h = &history[side_idx][ml.moves[i].fr][ml.moves[i].fc]
                                     [ml.moves[i].tr][ml.moves[i].tc];
                    *h += depth * depth;
                    if (*h > 50000) *h = 50000;
                }
            }
        }

        if (alpha >= beta) {
            /* Beta cutoff — update killers for quiet moves */
            if (!ml.moves[i].captured && !ml.moves[i].ant_eating &&
                depth < MAX_DEPTH) {
                if (!move_eq(&ml.moves[i], &killers[depth][0])) {
                    killers[depth][1] = killers[depth][0];
                    killers[depth][0] = ml.moves[i];
                }
            }
            flag = TT_BETA;
            break;
        }
    }

    /* No legal moves: checkmate or stalemate */
    if (legal_count == 0) {
        if (in_check_now)
            return -INF + (MAX_DEPTH - depth);
        return 0;
    }

    /* Store in transposition table (replace if deeper or same hash) */
    if (!search_stopped && (tte->hash != s->hash || tte->depth <= depth)) {
        tte->hash = s->hash;
        tte->depth = depth;
        tte->score = best_score;
        tte->flag = flag;
        tte->best = best_move;
        tte->valid = 1;
    }

    return best_score;
}

/* Evaluation from white's perspective (for score display) */
int evaluate_absolute(const State *s) {
    int rel = evaluate(s);
    return (s->side == WHITE) ? rel : -rel;
}

/* ---- Iterative Deepening with Root Move Ordering ---- */

static int g_max_depth_limit = 0; /* 0 = no limit */

/*
 * Search a position to given depth, return the score (side-to-move perspective).
 * Lightweight: no iterative deepening output, just the number.
 */
int ai_search_score(State *s, int time_ms, int max_depth) {
    g_max_depth_limit = max_depth;
    search_start = clock();
    search_time_limit = (time_ms > 0) ? time_ms : 999999999;
    search_stopped = 0;
    nodes_searched = 0;

    int score = 0;
    int dlimit = (max_depth > 0) ? max_depth : MAX_DEPTH;
    for (int d = 1; d <= dlimit; d++) {
        score = alpha_beta(s, d, -INF, INF, 1);
        if (search_stopped) break;
        if (score > INF - 100 || score < -INF + 100) break;
        if (time_ms > 0) {
            clock_t now = clock();
            int elapsed = (int)((now - search_start) * 1000 / CLOCKS_PER_SEC);
            if (elapsed > time_ms * 3 / 7) break;
        }
    }
    return score;
}

Move ai_best_move(State *s, int time_ms) {
    return ai_best_move_ex(s, time_ms, 0);
}

Move ai_best_move_ex(State *s, int time_ms, int max_depth) {
    /* Try opening book first */
    Move book_move;
    if (book_probe(s, &book_move)) {
        printf("AI: "); print_move(&book_move);
        printf("  (book move)\n");
        return book_move;
    }

    g_max_depth_limit = max_depth;
    MoveList ml;
    gen_legal_moves(s, &ml);

    if (ml.count == 0) {
        Move empty;
        memset(&empty, 0, sizeof(Move));
        return empty;
    }

    if (ml.count == 1) {
        printf("Only one legal move.\n");
        return ml.moves[0];
    }

    search_start = clock();
    search_time_limit = (time_ms > 0) ? time_ms : 999999999; /* 0 = unlimited */
    search_stopped = 0;
    nodes_searched = 0;

    /* Age history table between moves */
    {
        int a, b, c2, d, si;
        for (si = 0; si < 2; si++)
            for (a = 0; a < ROWS; a++)
                for (b = 0; b < COLS; b++)
                    for (c2 = 0; c2 < ROWS; c2++)
                        for (d = 0; d < COLS; d++)
                            history[si][a][b][c2][d] >>= 2;
    }

    Move best_move = ml.moves[0];
    int best_score = -INF;
    int depth;

    /* Root move scores for ordering between iterations */
    int root_scores[MAX_MOVES];
    memset(root_scores, 0, sizeof(root_scores));

    printf("AI thinking...\n");

    int depth_limit = (g_max_depth_limit > 0) ? g_max_depth_limit : MAX_DEPTH;

    for (depth = 1; depth <= depth_limit; depth++) {
        int alpha = -INF, beta = INF;

        /* Aspiration window for depth >= 5 */
        if (depth >= 5 && best_score > -INF + 100 && best_score < INF - 100) {
            alpha = best_score - 35;
            beta = best_score + 35;
        }

        int fail_count = 0;
aspiration_retry:
        ;

        Move iter_best = ml.moves[0];
        int iter_score = -INF;

        int i;
        for (i = 0; i < ml.count; i++) {
            State tmp;
            copy_state(&tmp, s);
            make_move(&tmp, &ml.moves[i]);

            int score;
            if (i == 0) {
                score = -alpha_beta(&tmp, depth - 1, -beta, -alpha, 1);
            } else {
                /* PVS at root */
                score = -alpha_beta(&tmp, depth - 1, -alpha - 1, -alpha, 1);
                if (score > alpha && score < beta) {
                    copy_state(&tmp, s);
                    make_move(&tmp, &ml.moves[i]);
                    score = -alpha_beta(&tmp, depth - 1, -beta, -alpha, 1);
                }
            }

            if (search_stopped) goto done;

            root_scores[i] = score;

            if (score > iter_score) {
                iter_score = score;
                iter_best = ml.moves[i];
            }
            if (score > alpha) alpha = score;
        }

        /* Aspiration window fail: widen and retry */
        if (depth >= 5 && fail_count < 2 &&
            (iter_score <= best_score - 35 || iter_score >= best_score + 35)) {
            alpha = -INF;
            beta = INF;
            fail_count++;
            goto aspiration_retry;
        }

        best_move = iter_best;
        best_score = iter_score;

        /* Reorder root moves: put best move first, sort rest by score */
        {
            int best_idx = 0;
            for (i = 0; i < ml.count; i++) {
                if (move_eq(&ml.moves[i], &best_move)) {
                    best_idx = i;
                    break;
                }
            }
            if (best_idx != 0) {
                Move tmp_m = ml.moves[0];
                int tmp_s = root_scores[0];
                ml.moves[0] = ml.moves[best_idx];
                root_scores[0] = root_scores[best_idx];
                ml.moves[best_idx] = tmp_m;
                root_scores[best_idx] = tmp_s;
            }
            /* Sort remaining by score (descending) */
            for (i = 2; i < ml.count; i++) {
                Move key_m = ml.moves[i];
                int key_s = root_scores[i];
                int j = i - 1;
                while (j >= 1 && root_scores[j] < key_s) {
                    ml.moves[j + 1] = ml.moves[j];
                    root_scores[j + 1] = root_scores[j];
                    j--;
                }
                ml.moves[j + 1] = key_m;
                root_scores[j + 1] = key_s;
            }
        }

        clock_t now = clock();
        int elapsed = (int)((now - search_start) * 1000 / CLOCKS_PER_SEC);

        printf("  depth %2d: score %+5d, nodes %8d, time %4dms  best=",
               depth, best_score, nodes_searched, elapsed);
        print_move(&best_move);
        printf("\n");

        if (best_score > INF - 100 || best_score < -INF + 100) break;
        /* Don't start next iteration if we've used enough time (skip if unlimited) */
        if (time_ms > 0 && elapsed > time_ms * 3 / 7) break;
    }

done:
    {
        clock_t now = clock();
        int elapsed = (int)((now - search_start) * 1000 / CLOCKS_PER_SEC);
        printf("AI: ");
        print_move(&best_move);
        printf("  (score %+d, %d nodes, %dms)\n",
               best_score, nodes_searched, elapsed);
    }

    return best_move;
}
