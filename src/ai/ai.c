#include "ai/ai.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "core/board.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/movegen.h"

#define AI_INF 100000000
#define AI_MATE 1000000
#define AI_Q_DEPTH 4 // Only for Alpha Version

// Piece-Square Table
static const int PST_ANT[ROWS][COLS] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

static const int PST_KNIGHT[ROWS][COLS] = {
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50}
};

static const int PST_BISHOP[ROWS][COLS] = {
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20},
    {-10, 10,  0,   0,   5,   5,   0,   0,   10,  -10},
    {-10, 10,  10,  10,  10,  10,  10,  10,  10,  -10},
    {-10, 0,   15,  20,  20,  20,  20,  15,  0,   -10},
    {-10, 5,   15,  20,  20,  20,  20,  15,  5,   -10},
    {-10, 0,   10,  15,  15,  15,  15,  10,  0,   -10},
    {-10, 5,   0,   0,   0,   0,   0,   0,   5,   -10},
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20}
};

static const int PST_ROOK[ROWS][COLS] = {
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 },
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {10, 15, 15, 15, 15, 15, 15, 15, 15, 10},
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 }
};

static const int PST_QUEEN[ROWS][COLS] = {
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20},
    {-10, 0,   5,   0,  0,  0,  0,  5,   0,   -10},
    {-10, 5,   5,   5,  5,  5,  5,  5,   5,   -10},
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-10, 0,   5,   5,  5,  5,  5,  5,   0,   -10},
    {-10, 0,   0,   0,  0,  0,  0,  0,   0,   -10},
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20}
};

static const int PST_KING_MID[ROWS][COLS] = {
    {20,  30,  10,  0,   0,   0,   0,   10,  30,  20 },
    {20,  20,  0,   -5,  -5,  -5,  -5,  0,   20,  20 },
    {-10, -20, -20, -20, -20, -20, -20, -20, -20, -10},
    {-20, -30, -30, -40, -40, -40, -40, -30, -30, -20},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30}
};

static const int PST_ANTEATER[ROWS][COLS] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

// Global variables for search
static clock_t g_search_start; // Start time of the search
static int g_time_limit_ms;    // Time limit in milliseconds
static int g_stop_search;      // Flag to stop the search, 1 = stop, 0 = continue
static int g_nodes;            // Number of nodes visited

// Piece Value Tables
static int piece_value(PieceType type) {
    switch (type) {
    case ANT:
        return 100;
    case KNIGHT:
        return 320;
    case BISHOP:
        return 330;
    case ROOK:
        return 500;
    case QUEEN:
        return 900;
    case KING:
        return 20000;
    case ANTEATER:
        return 200;
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

/* Helper function to reverse the PST for white
   Return the corespond row for the piece*/
static int board_row_for_pst(Color color, int row) {
    return (color == WHITE) ? (ROWS - 1 - row) : row;
}

/* Helper function to get the PST bonus for a piece
   Return the bonus for the piece*/
static int pst_bonus(Piece piece, int row, int col) {
    int pst_row; // get the corespond row

    pst_row = board_row_for_pst(piece.color, row);
    switch (piece.type) {
    case ANT:
        return PST_ANT[pst_row][col];
    case KNIGHT:
        return PST_KNIGHT[pst_row][col];
    case BISHOP:
        return PST_BISHOP[pst_row][col];
    case ROOK:
        return PST_ROOK[pst_row][col];
    case QUEEN:
        return PST_QUEEN[pst_row][col];
    case KING:
        return PST_KING_MID[pst_row][col];
    case ANTEATER:
        return PST_ANTEATER[pst_row][col];
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

// evaluate pawns
static int evaluate_pawns(const GameState *state, Color color) {
    int score = 0;
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));
            // init. the pawn's parameter
            int file_has_neighbor = 0;
            int file2;
            int row2;
            int is_passed = 1;
        }