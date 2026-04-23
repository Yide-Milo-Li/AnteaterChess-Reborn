#include "ai/ai.h"
#include "ai/book.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "core/board.h"
#include "core/hash.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/movegen.h"

// AI Constants
#define AI_INF 100000000       // Infinity
#define AI_MATE 1000000        // Mate score
#define AI_Q_DEPTH 8           // Quiescence search depth
#define AI_MAX_PLY 48          // Maximum number of ply
#define AI_MAX_PHASE 28        // 0 for initial, 28 for final stage
#define AI_HISTORY_MAX 2000000 // upper limit of history heuristic's score
#define ASPIRATION_WINDOW 60
#define NULL_MOVE_R 2

enum { TT_FLAG_EXACT = 0, TT_FLAG_LOWER = 1, TT_FLAG_UPPER = 2 };
typedef struct {
    uint64_t key; // position's zobrist hash
    int score;
    short depth;
    unsigned char flag;       // TT_FLAG_EXACT, TT_FLAG_LOWER, TT_FLAG_UPPER
    unsigned char generation; // search version
    unsigned char from;       // best move from
    unsigned char to;         // best move to
    unsigned char special;    // special move flag
} TTEntry;

typedef struct {
    clock_t searchStart;
    int timeLimitMs;
    int softTimeLimitMs;
    int stopSearch;
    int nodes;
    unsigned char generation;
    MoveList *moveBuffers;
    HashState hashStack[AI_MAX_PLY + 1]; // Length 49
    Move killerMoves[AI_MAX_PLY][2];
    unsigned char killerValid[AI_MAX_PLY][2];
    int history[2][ROWS * COLS][ROWS * COLS];
    uint64_t gameHashes[MAX_MOVES + 1];
    int gameHashCount;
    int gameHistoryStart;
    int repetitionLimit[AI_MAX_PLY + 1];
    unsigned char nullMoveActive[AI_MAX_PLY + 1];
} SearchContext;

static TTEntry g_transpositionTable[TT_SIZE];
static unsigned char g_ttGeneration;

/*
******************************
Piece-Square Table (PST)
******************************
*/

static const int PST_ANT[ROWS][COLS] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  0,  0,  5,  5,  5,  5,  0,  0,  0 },
    {5,  5,  10, 15, 20, 20, 15, 10, 5,  5 },
    {5,  10, 15, 25, 30, 30, 25, 15, 10, 5 },
    {10, 15, 25, 35, 40, 40, 35, 25, 15, 10},
    {20, 25, 35, 45, 50, 50, 45, 35, 25, 20},
    {50, 50, 50, 50, 50, 50, 50, 50, 50, 50},
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

static const int PST_KING_END[ROWS][COLS] = {
    {-50, -40, -30, -20, -20, -20, -20, -30, -40, -50},
    {-30, -15, -5,  5,   5,   5,   5,   -5,  -15, -30},
    {-20, -5,  10,  15,  20,  20,  15,  10,  -5,  -20},
    {-10, 0,   15,  25,  30,  30,  25,  15,  0,   -10},
    {-10, 0,   15,  25,  30,  30,  25,  15,  0,   -10},
    {-20, -5,  10,  15,  20,  20,  15,  10,  -5,  -20},
    {-30, -15, -5,  5,   5,   5,   5,   -5,  -15, -30},
    {-50, -40, -30, -20, -20, -20, -20, -30, -40, -50}
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

// Piece Value Tables
static int piece_value(PieceType type) {
    switch (type) {
    case ANT:
        return 100;
    case KNIGHT:
        return 320;
    case BISHOP:
        return 335;
    case ROOK:
        return 510;
    case QUEEN:
        return 950;
    case KING:
        return 20000;
    case ANTEATER:
        return 235;
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

// Phase evaluation table
// Modify must change AI_MAX_PHASE
static int phase_value(PieceType type) {
    switch (type) {
    case KNIGHT:
        return 1;
    case BISHOP:
        return 1;
    case ROOK:
        return 2;
    case QUEEN:
        return 4;
    case ANTEATER:
        return 1;
    case ANT:
    case KING:
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

// mobility score in piece
static int mobility_weight(PieceType type) {
    switch (type) {
    case ANT:
        return 4;
    case KNIGHT:
        return 5;
    case BISHOP:
        return 5;
    case ROOK:
        return 3;
    case QUEEN:
        return 2;
    case ANTEATER:
        return 6;
    case KING:
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

static int king_pst_bonus(Piece piece, int row, int col, int phase) {
    int pstRow;
    int midScore;
    int endScore;

    pstRow = board_row_for_pst(piece.color, row);
    midScore = PST_KING_MID[pstRow][col];
    endScore = PST_KING_END[pstRow][col];
    // Weighted average with phase = 0 is opening, phase = AI_MAX_PHASE is endgame
    return (midScore * phase + endScore * (AI_MAX_PHASE - phase)) / AI_MAX_PHASE;
}

/* Helper function to get the PST bonus for a piece
   Return the bonus for the piece*/
static int pst_bonus(Piece piece, int row, int col, int phase) {
    int pstRow;

    pstRow = board_row_for_pst(piece.color, row);
    switch (piece.type) {
    case ANT:
        return PST_ANT[pstRow][col];
    case KNIGHT:
        return PST_KNIGHT[pstRow][col];
    case BISHOP:
        return PST_BISHOP[pstRow][col];
    case ROOK:
        return PST_ROOK[pstRow][col];
    case QUEEN:
        return PST_QUEEN[pstRow][col];
    case KING:
        return king_pst_bonus(piece, row, col, phase);
    case ANTEATER:
        return PST_ANTEATER[pstRow][col];
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

static int is_promotion_move(const Move *move) {
    return move->specialType == PROMOTION_QUEEN || move->specialType == PROMOTION_ROOK ||
           move->specialType == PROMOTION_BISHOP || move->specialType == PROMOTION_KNIGHT ||
           move->specialType == PROMOTION_ANTEATER;
}

static int is_noisy_move(const Move *move) {
    return move->captureCount > 0 || move->specialType == ANTEATER_CAPTURE || is_promotion_move(move);
}

static int is_quiet_move(const Move *move) {
    return !is_noisy_move(move) && move->specialType != CASTLING_KINGSIDE && move->specialType != CASTLING_QUEENSIDE;
}

static int square_index(Position pos) {
    return pos.row * COLS + pos.col;
}

// abs function for efficiency
static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

// irreversile move (cannot be undone)
static int is_irreversible_move(const Move *move) {
    if (move == NULL) {
        return 0;
    }

    return move->movedPiece.type == ANT || move->captureCount > 0 || is_promotion_move(move);
}

// SEE general path check
// Return 1 if path is clear, 0 if blocked
// DO NOT USE WHEN NOT IN THE SAME ROW OR COLUMN OR DIAGONAL
// DESTINATION CANNOT BE THE SAME AS FROM
static int is_path_clear_for_attack(const Board *board, Position from, Position target) {
    int rowStep;
    int colStep;
    Position current;

    rowStep = 0;
    colStep = 0;

    // Determine the direction of the attack
    if (target.row > from.row) {
        rowStep = 1;
    } else if (target.row < from.row) {
        rowStep = -1;
    }

    if (target.col > from.col) {
        colStep = 1;
    } else if (target.col < from.col) {
        colStep = -1;
    }

    current = from;
    current.row += rowStep;
    current.col += colStep;

    while (!positionEqual(current, target)) {
        if (getPiece(board, current).type != EMPTY_PIECE) {
            return 0;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 1;
}

static int ant_attacks_square(Position from, Piece piece, Position target) {
    int direction;

    direction = (piece.color == WHITE) ? -1 : 1;

    // target is in correct row direction and in diagonal position
    return (target.row - from.row) == direction && absolute_value(target.col - from.col) == 1;
}

static int rook_attacks_square(const Board *board, Position from, Position target) {
    // target not in same column or row
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    // call helper function to check if path is clear (without including diagonal)
    return is_path_clear_for_attack(board, from, target);
}