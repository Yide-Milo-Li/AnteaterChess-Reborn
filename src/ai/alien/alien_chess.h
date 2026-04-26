#ifndef CHESS_H
#define CHESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>

/* Board dimensions: 10 columns (A-J) x 8 rows (1-8) */
#define ROWS 8
#define COLS 10

/* Piece types (absolute values) */
#define EMPTY    0
#define PAWN     1   /* also called "ant" */
#define KNIGHT   2
#define BISHOP   3
#define ROOK     4
#define QUEEN    5
#define KING     6
#define ANTEATER 7
#define NUM_TYPES 8

/* Colors */
#define WHITE 1
#define BLACK 2

/*
 * Piece encoding on the board:
 *   0        = empty square
 *   +1..+7   = white piece (PAWN..ANTEATER)
 *   -1..-7   = black piece
 */
#define PIECE_COLOR(p)  ((p) > 0 ? WHITE : ((p) < 0 ? BLACK : 0))
#define PIECE_TYPE(p)   ((p) > 0 ? (p) : -(p))
#define MAKE_PIECE(t,c) ((c) == WHITE ? (t) : -(t))
#define IS_EMPTY(p)     ((p) == 0)
#define OPP_COLOR(c)    ((c) == WHITE ? BLACK : WHITE)

/* Initial positions */
#define WHITE_KING_ROW 0
#define WHITE_KING_COL 5
#define BLACK_KING_ROW 7
#define BLACK_KING_COL 5

/* Move limits */
#define MAX_MOVES  512
#define MAX_CHAIN  20
#define MAX_GAME   2000
#define MAX_DEPTH  40

/* Infinity for search */
#define INF 999999

/* Transposition table */
#define TT_SIZE (1 << 20)  /* ~1M entries */
#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA  2

/* ---- Data Structures ---- */

typedef struct {
    int fr, fc;           /* from row, col */
    int tr, tc;           /* to row, col */
    int captured;         /* captured piece value (0 if none) */
    int promotion;        /* promotion type (0 if none) */
    int castle;           /* 0=none, 1=kingside, 2=queenside */
    int en_passant;       /* 1 if en passant capture */
    int ant_eating;       /* 1 if ant eating chain move */
    int chain_len;        /* number of captures in chain */
    int chain_r[MAX_CHAIN];
    int chain_c[MAX_CHAIN];
    int chain_cap[MAX_CHAIN]; /* piece at each chain pos */
    int score;            /* move ordering score */
} Move;

typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

typedef struct {
    int board[ROWS][COLS];
    int side;             /* WHITE or BLACK to move */
    /* Castling: 0 = right available, 1 = right lost */
    int wk_moved;         /* white king has moved */
    int bk_moved;         /* black king has moved */
    int wra_moved;        /* white A-file rook moved */
    int wrj_moved;        /* white J-file rook moved */
    int bra_moved;        /* black A-file rook moved */
    int brj_moved;        /* black J-file rook moved */
    int ep_col;           /* en passant target column, -1 if none */
    int wk_r, wk_c;      /* white king position */
    int bk_r, bk_c;      /* black king position */
    int move_num;         /* full move number */
    int halfmove;         /* halfmove clock for 50-move rule */
    uint64_t hash;        /* Zobrist hash */
} State;

/* Game history entry */
typedef struct {
    State state;
    Move move;
} HistoryEntry;

/* Transposition table entry */
typedef struct {
    uint64_t hash;
    int depth;
    int score;
    int flag;             /* TT_EXACT, TT_ALPHA, TT_BETA */
    Move best;
    int valid;
} TTEntry;

/* ---- Zobrist Hashing ---- */
extern uint64_t zobrist_piece[ROWS][COLS][15]; /* index = piece+7 */
extern uint64_t zobrist_side;
extern uint64_t zobrist_castle[6];
extern uint64_t zobrist_ep[COLS];

/* ---- Function Declarations ---- */

/* board.c */
void    init_board(State *s);
void    copy_state(State *dst, const State *src);
int     valid_pos(int r, int c);
void    print_board(const State *s, int flip);
char    piece_char(int p);
void    pos_to_str(int r, int c, char *buf);
int     parse_pos(const char *str, int *r, int *c);
void    init_zobrist(void);
uint64_t compute_hash(const State *s);

/* movegen.c */
void    gen_pseudo_moves(const State *s, MoveList *ml);
void    gen_legal_moves(const State *s, MoveList *ml);
int     is_attacked(const State *s, int r, int c, int by_color);
int     in_check(const State *s, int color);
int     is_checkmate(const State *s);
int     is_stalemate(const State *s);
void    make_move(State *s, const Move *m);
int     is_legal_move(const State *s, const Move *m);

/* ai.c */
void    ai_init(void);
Move    ai_best_move(State *s, int time_ms);
Move    ai_best_move_ex(State *s, int time_ms, int max_depth);
int     ai_search_score(State *s, int time_ms, int max_depth); /* returns score */
int     evaluate(const State *s);
int     evaluate_absolute(const State *s); /* always from white's perspective */

/* ui.c */
void    display_welcome(void);
int     choose_color(void);
int     choose_mode(void);
int     get_human_move(const State *s, Move *m);
void    print_move(const Move *m);
void    print_move_str(const Move *m, char *buf, int bufsz);

/* book.c */
int     book_load_global(const char *filename);
int     book_load_neural(const char *filename);
void    book_set_mode(int mode);
int     book_get_mode(void);
int     book_probe(const State *s, Move *move);

/* log.c */
void    log_open(const char *filename);
void    log_move_entry(const Move *m, int move_num, int side);
void    log_result(const char *result);
void    log_close(void);

#endif /* CHESS_H */
