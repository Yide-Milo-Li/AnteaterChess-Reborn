#include "alien_namespace.h"
#include "alien_chess.h"

/* Zobrist random numbers */
uint64_t zobrist_piece[ROWS][COLS][15];
uint64_t zobrist_side;
uint64_t zobrist_castle[6];
uint64_t zobrist_ep[COLS];

/* Simple xorshift64 PRNG for Zobrist init */
static uint64_t xorshift_state = 0x12345678ABCDEF01ULL;
static uint64_t xorshift64(void) {
    uint64_t x = xorshift_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    xorshift_state = x;
    return x;
}

void init_zobrist(void) {
    int r, c, p;
    xorshift_state = 0x12345678ABCDEF01ULL; /* reset seed for deterministic hashes */
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            for (p = 0; p < 15; p++)
                zobrist_piece[r][c][p] = xorshift64();
    zobrist_side = xorshift64();
    for (p = 0; p < 6; p++)
        zobrist_castle[p] = xorshift64();
    for (c = 0; c < COLS; c++)
        zobrist_ep[c] = xorshift64();
}

uint64_t compute_hash(const State *s) {
    uint64_t h = 0;
    int r, c;
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            if (s->board[r][c] != EMPTY)
                h ^= zobrist_piece[r][c][s->board[r][c] + 7];
    if (s->side == BLACK) h ^= zobrist_side;
    if (s->wk_moved)  h ^= zobrist_castle[0];
    if (s->wra_moved)  h ^= zobrist_castle[1];
    if (s->wrj_moved)  h ^= zobrist_castle[2];
    if (s->bk_moved)  h ^= zobrist_castle[3];
    if (s->bra_moved)  h ^= zobrist_castle[4];
    if (s->brj_moved)  h ^= zobrist_castle[5];
    if (s->ep_col >= 0) h ^= zobrist_ep[s->ep_col];
    return h;
}

/*
 * Initial board layout for Anteater Chess (10 cols x 8 rows):
 *   Row 8 (idx 7): bR bN bB bA bQ bK bA bB bN bR
 *   Row 7 (idx 6): bP bP bP bP bP bP bP bP bP bP
 *   Rows 3-6 (idx 2-5): empty
 *   Row 2 (idx 1): wP wP wP wP wP wP wP wP wP wP
 *   Row 1 (idx 0): wR wN wB wA wQ wK wA wB wN wR
 */
void init_board(State *s) {
    int r, c;
    memset(s, 0, sizeof(State));

    /* Clear board */
    for (r = 0; r < ROWS; r++)
        for (c = 0; c < COLS; c++)
            s->board[r][c] = EMPTY;

    /* White pieces (row 0 = rank 1) */
    s->board[0][0] = MAKE_PIECE(ROOK, WHITE);
    s->board[0][1] = MAKE_PIECE(KNIGHT, WHITE);
    s->board[0][2] = MAKE_PIECE(BISHOP, WHITE);
    s->board[0][3] = MAKE_PIECE(ANTEATER, WHITE);
    s->board[0][4] = MAKE_PIECE(QUEEN, WHITE);
    s->board[0][5] = MAKE_PIECE(KING, WHITE);
    s->board[0][6] = MAKE_PIECE(ANTEATER, WHITE);
    s->board[0][7] = MAKE_PIECE(BISHOP, WHITE);
    s->board[0][8] = MAKE_PIECE(KNIGHT, WHITE);
    s->board[0][9] = MAKE_PIECE(ROOK, WHITE);

    /* White pawns (row 1 = rank 2) */
    for (c = 0; c < COLS; c++)
        s->board[1][c] = MAKE_PIECE(PAWN, WHITE);

    /* Black pawns (row 6 = rank 7) */
    for (c = 0; c < COLS; c++)
        s->board[6][c] = MAKE_PIECE(PAWN, BLACK);

    /* Black pieces (row 7 = rank 8) */
    s->board[7][0] = MAKE_PIECE(ROOK, BLACK);
    s->board[7][1] = MAKE_PIECE(KNIGHT, BLACK);
    s->board[7][2] = MAKE_PIECE(BISHOP, BLACK);
    s->board[7][3] = MAKE_PIECE(ANTEATER, BLACK);
    s->board[7][4] = MAKE_PIECE(QUEEN, BLACK);
    s->board[7][5] = MAKE_PIECE(KING, BLACK);
    s->board[7][6] = MAKE_PIECE(ANTEATER, BLACK);
    s->board[7][7] = MAKE_PIECE(BISHOP, BLACK);
    s->board[7][8] = MAKE_PIECE(KNIGHT, BLACK);
    s->board[7][9] = MAKE_PIECE(ROOK, BLACK);

    s->side = WHITE;
    s->wk_moved = s->bk_moved = 0;
    s->wra_moved = s->wrj_moved = 0;
    s->bra_moved = s->brj_moved = 0;
    s->ep_col = -1;
    s->wk_r = 0; s->wk_c = 5;
    s->bk_r = 7; s->bk_c = 5;
    s->move_num = 1;
    s->halfmove = 0;
    s->hash = compute_hash(s);
}

void copy_state(State *dst, const State *src) {
    memcpy(dst, src, sizeof(State));
}

int valid_pos(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

char piece_char(int p) {
    int t = PIECE_TYPE(p);
    char ch;
    switch (t) {
        case PAWN:     ch = 'P'; break;
        case KNIGHT:   ch = 'N'; break;
        case BISHOP:   ch = 'B'; break;
        case ROOK:     ch = 'R'; break;
        case QUEEN:    ch = 'Q'; break;
        case KING:     ch = 'K'; break;
        case ANTEATER: ch = 'A'; break;
        default:       return ' ';
    }
    return ch;
}

void pos_to_str(int r, int c, char *buf) {
    buf[0] = 'A' + c;
    buf[1] = '1' + r;
    buf[2] = '\0';
}

int parse_pos(const char *str, int *r, int *c) {
    char file = toupper(str[0]);
    char rank = str[1];
    if (file < 'A' || file > 'J') return 0;
    if (rank < '1' || rank > '8') return 0;
    *c = file - 'A';
    *r = rank - '1';
    return 1;
}

void print_board(const State *s, int flip) {
    int r, c, p;
    char color_ch;

    printf("\n");
    if (flip) {
        /* Print from white's perspective (row 1 at bottom) already default */
        for (r = ROWS - 1; r >= 0; r--) {
            printf("   +----+----+----+----+----+----+----+----+----+----+\n");
            printf(" %d ", r + 1);
            for (c = 0; c < COLS; c++) {
                p = s->board[r][c];
                if (IS_EMPTY(p)) {
                    printf("|    ");
                } else {
                    color_ch = (PIECE_COLOR(p) == WHITE) ? 'w' : 'b';
                    printf("| %c%c ", color_ch, piece_char(p));
                }
            }
            printf("|\n");
        }
    } else {
        for (r = ROWS - 1; r >= 0; r--) {
            printf("   +----+----+----+----+----+----+----+----+----+----+\n");
            printf(" %d ", r + 1);
            for (c = 0; c < COLS; c++) {
                p = s->board[r][c];
                if (IS_EMPTY(p)) {
                    printf("|    ");
                } else {
                    color_ch = (PIECE_COLOR(p) == WHITE) ? 'w' : 'b';
                    printf("| %c%c ", color_ch, piece_char(p));
                }
            }
            printf("|\n");
        }
    }
    printf("   +----+----+----+----+----+----+----+----+----+----+\n");
    printf("     A    B    C    D    E    F    G    H    I    J\n\n");

    printf("   %s to move", s->side == WHITE ? "White" : "Black");
    if (s->ep_col >= 0)
        printf("  [en passant on col %c]", 'A' + s->ep_col);
    printf("  [move %d]\n\n", s->move_num);
}
