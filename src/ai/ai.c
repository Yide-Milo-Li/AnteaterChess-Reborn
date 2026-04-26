
// Plugin for Experimental Difficulty
#if defined(__has_include)
#if __has_include("ai/alien/alien_engine.h")
#include "ai/alien/alien_engine.h"
#define HAS_ALIEN_PLUGIN 1
#else
#define HAS_ALIEN_PLUGIN 0
#endif
#else
#define HAS_ALIEN_PLUGIN 0
#endif

#include "ai/ai.h"
#include "ai/book.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/hash.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/movegen.h"
#include "time/clock.h"

// AI Constants
#define AI_INF 100000000       // Infinity
#define AI_MATE 1000000        // Mate score
#define AI_Q_DEPTH 8           // Quiescence search depth
#define AI_MAX_PLY 48          // Maximum number of ply
#define AI_MAX_PHASE 28        // 28 for initial, 0 for final stage
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
    int64_t searchStartMs;
    int timeLimitMs;
    int softTimeLimitMs;
    int stopSearch;
    int nodes;
    unsigned char generation;
    MoveList *moveBuffers;
    HashState hashStack[AI_MAX_PLY + 1]; // Length 49
    uint64_t gameHashes[MAX_MOVES + 1];
    int gameHashCount;
    int gameHistoryStart;
    int repetitionLimit[AI_MAX_PLY + 1];
    unsigned char nullMoveActive[AI_MAX_PLY + 1];
} SearchContext;

static TTEntry g_transpositionTable[TT_SIZE];
static unsigned char g_ttGeneration;
static Move g_killerMoves[AI_MAX_PLY][2];
static unsigned char g_killerValid[AI_MAX_PLY][2];
static int g_history[2][ROWS * COLS][ROWS * COLS];
static unsigned char g_searchHeuristicsReady;

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
    {15, 15, 15, 15, 15, 15, 15, 15, 15, 15},
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {15, 15, 15, 15, 15, 15, 15, 15, 15, 15}
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
    return move != NULL && isPromotionSpecialMove(move->specialType);
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

static int bishop_attacks_square(const Board *board, Position from, Position target) {
    if (absolute_value(target.row - from.row) != absolute_value(target.col - from.col)) {
        return 0;
    }
    return is_path_clear_for_attack(board, from, target);
}

static int knight_attacks_square(Position from, Position target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
}

static int king_attacks_square(Position from, Position target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !positionEqual(from, target);
}

// Actually not correct, just for SEE
static int anteater_attacks_piece_for_see(const Board *board, Position from, Piece piece, Position target,
                                          Piece targetPiece) {
    int rowDistance;
    int colDistance;
    int rowStep;
    int colStep;
    Position current;

    if (piece.type != ANTEATER || targetPiece.type != ANT) {
        return 0;
    }

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    if (rowDistance <= 1 && colDistance <= 1 && !positionEqual(from, target)) {
        return 1;
    }

    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    rowStep = 0;
    colStep = 0;
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

    current = createPosition(from.row + rowStep, from.col + colStep);
    while (isValidPosition(current)) {
        Piece occupant = getPiece(board, current);

        if (occupant.type != ANT || occupant.color == piece.color) {
            return 0;
        }
        if (positionEqual(current, target)) {
            return 1;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 0;
}

static int piece_attacks_square_for_see(const Board *board, Position from, Piece piece, Position target,
                                        Piece targetPiece) {
    switch (piece.type) {
    case ANT:
        return ant_attacks_square(from, piece, target);
    case ROOK:
        return rook_attacks_square(board, from, target);
    case KNIGHT:
        return knight_attacks_square(from, target);
    case BISHOP:
        return bishop_attacks_square(board, from, target);
    case QUEEN:
        return rook_attacks_square(board, from, target) || bishop_attacks_square(board, from, target);
    case KING:
        return king_attacks_square(from, target);
    case ANTEATER:
        return anteater_attacks_piece_for_see(board, from, piece, target, targetPiece);
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

static int square_is_attacked_for_king(const Board *board, Position target, Color attackingColor) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(board, from);
            Piece kingTarget = createPiece(KING, (attackingColor == WHITE) ? BLACK : WHITE);

            if (piece.type == EMPTY_PIECE || piece.color != attackingColor) {
                continue;
            }
            if (piece.type == ANTEATER) {
                continue;
            }
            if (piece_attacks_square_for_see(board, from, piece, target, kingTarget) == 1) {
                return 1;
            }
        }
    }

    return 0;
}

// For SEE in case of promotion
static Piece piece_after_see_capture(Piece piece, Position target) {
    if (piece.type == ANT &&
        ((piece.color == WHITE && target.row == 0) || (piece.color == BLACK && target.row == ROWS - 1))) {
        return createPiece(QUEEN, piece.color);
    }

    return piece;
}

static void apply_move_to_board_for_see(Board *board, Move move) {
    Piece placedPiece;
    int captureIndex;

    // remove the attacker and all capture pieces from the board
    removePiece(board, move.from);
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        removePiece(board, move.captures[captureIndex].pos);
    }

    placedPiece = move.movedPiece;

    // in case of promotion, change the piece type
    if (move.specialType == PROMOTION_QUEEN) {
        placedPiece = createPiece(QUEEN, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_ROOK) {
        placedPiece = createPiece(ROOK, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_BISHOP) {
        placedPiece = createPiece(BISHOP, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_KNIGHT) {
        placedPiece = createPiece(KNIGHT, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_ANTEATER) {
        placedPiece = createPiece(ANTEATER, move.movedPiece.color);
    }

    setPiece(board, move.to, placedPiece);
}

// In the SEE simulation, king cannot be captured
static int king_capture_is_legal_for_see(const Board *board, Position from, Piece piece, Position target) {
    Board trial;
    Color enemyColor;

    trial = *board;
    removePiece(&trial, from);
    setPiece(&trial, target, piece);
    enemyColor = (piece.color == WHITE) ? BLACK : WHITE;
    return square_is_attacked_for_king(&trial, target, enemyColor) == 0;
}

static int find_least_valuable_attacker(const Board *board, Position target, Color side, Position *fromOut,
                                        Piece *pieceOut, int *valueOut) {
    int row;
    int col;
    int bestValue;
    int found;
    Piece targetPiece;

    if (board == NULL || fromOut == NULL || pieceOut == NULL || valueOut == NULL) {
        return 0;
    }

    targetPiece = getPiece(board, target);
    bestValue = AI_INF; // set to infinity to get the lowest value
    found = 0;
    // iterate all board to find the least valuable attacker
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(board, from);
            int value;

            // skip EMPTY
            if (piece.type == EMPTY_PIECE || piece.color != side) {
                continue;
            }
            // skip KING if it will be captured in case of capture it
            if (piece.type == KING && king_capture_is_legal_for_see(board, from, piece, target) == 0) {
                continue;
            }
            // skip if the piece does not attack the target
            if (piece_attacks_square_for_see(board, from, piece, target, targetPiece) == 0) {
                continue;
            }

            value = piece_value(piece.type);
            if (!found || value < bestValue) {
                *fromOut = from;
                *pieceOut = piece;
                *valueOut = value;
                bestValue = value;
                found = 1;
            }
        }
    }

    return found;
}

static int see_initial_gain(const Move *move) {
    int gain;
    int captureIndex;

    gain = 0;
    // sum all the value of the captured pieces
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        gain += piece_value(move->captures[captureIndex].piece.type);
    }
    // if promotion, add the value of the new piece
    if (is_promotion_move(move)) {
        gain += piece_value(QUEEN) - piece_value(ANT);
    }

    return gain;
}

//*****************************************************************
// MAIN STATIC EXCHANGE EVALUATION FUNCTION
//*****************************************************************

static int see_move_score(const GameState *state, const Move *move) {
    Board board;
    Position target;
    Color side;
    int gain[32];
    int depth;
    Piece occupant;

    if (state == NULL || move == NULL || !is_noisy_move(move)) {
        return 0;
    }

    board = state->board;                                     // get a copy of the board
    target = move->to;                                        // get the target position
    gain[0] = see_initial_gain(move);                         // get the initial gain
    apply_move_to_board_for_see(&board, *move);               // apply the move to the copy
    occupant = getPiece(&board, target);                      // get the piece at the target
    side = (move->movedPiece.color == WHITE) ? BLACK : WHITE; // get the next attack
    depth = 1;

    // set limitation as 32
    while (depth < (int)(sizeof(gain) / sizeof(gain[0]))) {
        Position from;
        Piece attacker;
        int attackerValue;

        // find the least valuable attacker
        if (find_least_valuable_attacker(&board, target, side, &from, &attacker, &attackerValue) == 0) {
            break;
        }

        gain[depth] = piece_value(occupant.type) - gain[depth - 1];

        // early pruning if both side losing material
        if ((gain[depth] < 0) && (-gain[depth - 1] < 0)) {
            break;
        }

        // apply the move to the copy
        removePiece(&board, from);
        occupant = piece_after_see_capture(attacker, target);
        setPiece(&board, target, occupant);
        side = (side == WHITE) ? BLACK : WHITE;
        ++depth;
    }

    // trace back
    while (--depth > 0) {
        if (-gain[depth] < gain[depth - 1]) {
            gain[depth - 1] = -gain[depth];
        }
    }

    return gain[0];
}

//*****************************************************************
// PIECE MOBILITY
//*****************************************************************

// get the sliding mobility, in one direction
static int count_sliding_mobility(const Board *board, Position from, Piece piece, int rowStep, int colStep) {
    Position current;
    int mobility;

    current = from;
    current.row += rowStep;
    current.col += colStep;
    mobility = 0;
    while (isValidPosition(current)) {
        Piece target = getPiece(board, current);

        if (target.type == EMPTY_PIECE) {
            ++mobility;
        } else {
            if (target.color != piece.color) {
                ++mobility;
            }
            break;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return mobility;
}

static int count_ant_mobility(const Board *board, Position from, Piece piece) {
    int direction;
    int mobility;
    Position forward;
    int fileOffset;

    direction = (piece.color == WHITE) ? -1 : 1;
    mobility = 0;

    // one step forward
    forward = createPosition(from.row + direction, from.col);
    if (isValidPosition(forward) && getPiece(board, forward).type == EMPTY_PIECE) {
        ++mobility;

        // two step forward
        if (((piece.color == WHITE && from.row == 6) || (piece.color == BLACK && from.row == 1))) {
            Position doubleStep = createPosition(from.row + (2 * direction), from.col);

            if (isValidPosition(doubleStep) && getPiece(board, doubleStep).type == EMPTY_PIECE) {
                ++mobility;
            }
        }
    }

    // diagonal
    for (fileOffset = -1; fileOffset <= 1; fileOffset += 2) {
        Position diagonal = createPosition(from.row + direction, from.col + fileOffset);
        Piece target;

        if (!isValidPosition(diagonal)) {
            continue;
        }

        target = getPiece(board, diagonal);
        if (target.type != EMPTY_PIECE && target.color != piece.color) {
            ++mobility;
        }
    }

    return mobility;
}

static int count_knight_mobility(const Board *board, Position from, Piece piece) {
    // knight moves
    static const int rowOffsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    static const int colOffsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};
    int index;
    int mobility;

    mobility = 0;
    for (index = 0; index < 8; ++index) {
        Position target = createPosition(from.row + rowOffsets[index], from.col + colOffsets[index]);
        Piece occupant;

        if (!isValidPosition(target)) {
            continue;
        }

        occupant = getPiece(board, target);
        if (occupant.type == EMPTY_PIECE || occupant.color != piece.color) {
            ++mobility;
        }
    }

    return mobility;
}

static int count_anteater_mobility(const Board *board, Position from, Piece piece) {
    // first step, all 8 directions
    static const int rowSteps[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int colSteps[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    // chain step, up, down, left, right. Its a approximation
    static const int chainRowSteps[] = {-1, 1, 0, 0};
    static const int chainColSteps[] = {0, 0, -1, 1};
    int mobility;
    int index;

    mobility = 0;
    for (index = 0; index < 8; ++index) {
        Position target = createPosition(from.row + rowSteps[index], from.col + colSteps[index]);
        Piece occupant;

        if (!isValidPosition(target)) {
            continue;
        }

        occupant = getPiece(board, target);
        if (occupant.type == EMPTY_PIECE || (occupant.type == ANT && occupant.color != piece.color)) {
            ++mobility;
        }
    }

    for (index = 0; index < 4; ++index) {
        Position current = createPosition(from.row + chainRowSteps[index], from.col + chainColSteps[index]);
        int chainLength;

        chainLength = 0;
        while (isValidPosition(current)) {
            Piece target = getPiece(board, current);

            if (target.type != ANT || target.color == piece.color) {
                break;
            }

            ++chainLength;
            if (chainLength >= 2) {
                ++mobility;
            }

            current.row += chainRowSteps[index];
            current.col += chainColSteps[index];
        }
    }

    return mobility;
}

static int count_king_mobility(const Board *board, Position from, Piece piece) {
    int rowOffset;
    int colOffset;
    int mobility;

    mobility = 0;
    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            Position target;
            Piece occupant;

            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            target = createPosition(from.row + rowOffset, from.col + colOffset);
            if (!isValidPosition(target)) {
                continue;
            }

            occupant = getPiece(board, target);
            if (occupant.type == EMPTY_PIECE || occupant.color != piece.color) {
                ++mobility;
            }
        }
    }

    return mobility;
}

static int piece_mobility(const Board *board, Position from, Piece piece) {
    switch (piece.type) {
    case ANT:
        return count_ant_mobility(board, from, piece);
    case ROOK:
        // 4 sliding directions
        return count_sliding_mobility(board, from, piece, -1, 0) + count_sliding_mobility(board, from, piece, 1, 0) +
               count_sliding_mobility(board, from, piece, 0, -1) + count_sliding_mobility(board, from, piece, 0, 1);
    case BISHOP:
        // 4 sliding directions
        return count_sliding_mobility(board, from, piece, -1, -1) + count_sliding_mobility(board, from, piece, -1, 1) +
               count_sliding_mobility(board, from, piece, 1, -1) + count_sliding_mobility(board, from, piece, 1, 1);
    case QUEEN:
        // 8 sliding directions
        return count_sliding_mobility(board, from, piece, -1, 0) + count_sliding_mobility(board, from, piece, 1, 0) +
               count_sliding_mobility(board, from, piece, 0, -1) + count_sliding_mobility(board, from, piece, 0, 1) +
               count_sliding_mobility(board, from, piece, -1, -1) + count_sliding_mobility(board, from, piece, -1, 1) +
               count_sliding_mobility(board, from, piece, 1, -1) + count_sliding_mobility(board, from, piece, 1, 1);
    case KNIGHT:
        return count_knight_mobility(board, from, piece);
    case KING:
        return count_king_mobility(board, from, piece);
    case ANTEATER:
        return count_anteater_mobility(board, from, piece);
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

static int ant_supports_square(const Board *board, Color color, Position target) {
    int supportRow;
    int offset;

    supportRow = target.row + ((color == WHITE) ? 1 : -1);
    for (offset = -1; offset <= 1; offset += 2) {
        Position support = createPosition(supportRow, target.col + offset);
        Piece piece;

        if (!isValidPosition(support)) {
            continue;
        }

        piece = getPiece(board, support);
        if (piece.type == ANT && piece.color == color) {
            return 1;
        }
    }

    return 0;
}

static int enemy_ant_can_attack_square(const Board *board, Color color, Position target) {
    Color enemyColor;
    int enemyRow;
    int offset;

    enemyColor = (color == WHITE) ? BLACK : WHITE;
    enemyRow = target.row + ((color == WHITE) ? -1 : 1);
    for (offset = -1; offset <= 1; offset += 2) {
        Position attacker = createPosition(enemyRow, target.col + offset);
        Piece piece;

        if (!isValidPosition(attacker)) {
            continue;
        }

        piece = getPiece(board, attacker);
        if (piece.type == ANT && piece.color == enemyColor) {
            return 1;
        }
    }

    return 0;
}

// Evaluate pawns cuuurent implenmnet doesn't consider anteaters
static int evaluate_pawns(const GameState *state, Color color, const int antFiles[2][COLS], int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));
            int adjacentFriends;
            int isPassed;
            int step;
            int advance;
            int passedBonus;

            // Get all alley ants
            if (piece.type != ANT || piece.color != color) {
                continue;
            }

            // antFiles[color][col]: the number of color ants in the same file
            // Double pawns penalty
            if (antFiles[color][col] > 1) {
                score -= 12 * (antFiles[color][col] - 1);
            }

            // penalty for isolated ants and bonus for connected ants
            adjacentFriends = 0;
            if (col > 0 && antFiles[color][col - 1] > 0) {
                adjacentFriends = 1;
            }
            if (col + 1 < COLS && antFiles[color][col + 1] > 0) {
                adjacentFriends = 1;
            }
            if (!adjacentFriends) {
                score -= 10;
            } else {
                score += 6;
            }

            // check passed pawn
            isPassed = 1;
            if (color == WHITE) {
                for (step = row - 1; step >= 0 && isPassed; --step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        Piece enemy;

                        if (file < 0 || file >= COLS) {
                            continue;
                        }

                        enemy = getPiece(&state->board, createPosition(step, file));
                        if (enemy.type == ANT && enemy.color == BLACK) {
                            isPassed = 0;
                            break;
                        }
                    }
                }
                advance = ROWS - 1 - row;
            } else {
                for (step = row + 1; step < ROWS && isPassed; ++step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        Piece enemy;

                        if (file < 0 || file >= COLS) {
                            continue;
                        }

                        enemy = getPiece(&state->board, createPosition(step, file));
                        if (enemy.type == ANT && enemy.color == WHITE) {
                            isPassed = 0;
                            break;
                        }
                    }
                }
                advance = row;
            }

            // bonus for passed pawn
            if (isPassed) {
                passedBonus = 18 + advance * 12;
                if (phase <= 12) {
                    passedBonus += 10 + advance * 4;
                }
                if (adjacentFriends) {
                    passedBonus += 8;
                }
                score += passedBonus;
            }
        }
    }

    return score;
}

static int evaluate_rook_and_queen_files(const GameState *state, Color color, const int antFiles[2][COLS], int phase) {
    int score;
    int row;
    int col;
    Color enemyColor;

    (void)phase;

    score = 0;
    enemyColor = (color == WHITE) ? BLACK : WHITE;
    // iterate over all rooks and queens
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));

            if (piece.color != color || (piece.type != ROOK && piece.type != QUEEN)) {
                continue;
            }

            // If file without any ants , add bonus
            if (antFiles[color][col] == 0 && antFiles[enemyColor][col] == 0) {
                score += (piece.type == ROOK) ? 20 : 8;
                // If file without any alley ants , add bonus
            } else if (antFiles[color][col] == 0) {
                score += (piece.type == ROOK) ? 12 : 4;
            }

            // Bonus for rook on 7th rank
            if (piece.type == ROOK) {
                if ((color == WHITE && row == 1) || (color == BLACK && row == 6)) {
                    score += 14;
                }
            }
        }
    }

    return score;
}

static int evaluate_king_safety(const GameState *state, Color color, Position kingPos, int phase) {
    int score;
    int homeRow;
    int shieldRow;
    int colOffset;

    if (!isValidPosition(kingPos)) {
        return 0;
    }

    score = 0;
    homeRow = (color == WHITE) ? 7 : 0;
    // Bonus for Castling
    if (kingPos.row == homeRow && (kingPos.col == 3 || kingPos.col == 7)) {
        score += 32;
        // game ending penalty for king on default position
    } else if (phase >= 18 && kingPos.row == homeRow && kingPos.col == 5) {
        score -= 24;
    }

    // Pawn Shield
    shieldRow = kingPos.row + ((color == WHITE) ? -1 : 1);
    for (colOffset = -1; colOffset <= 1; ++colOffset) {
        Position shield = createPosition(shieldRow, kingPos.col + colOffset);
        Piece occupant;

        if (!isValidPosition(shield)) {
            continue;
        }

        occupant = getPiece(&state->board, shield);
        if (occupant.type == ANT && occupant.color == color) {
            score += 12;
        } else {
            score -= 8;
        }
    }

    // mobility bonus in end game
    if (phase <= 10) {
        score += count_king_mobility(&state->board, kingPos, createPiece(KING, color)) * 2;
    }

    return score;
}

static int evaluate_development(const GameState *state, Color color, int phase) {
    int score;
    int homeRow;
    int undevelopedMinors;
    int col;

    // Only evaluate in opening and midgame
    if (state->moveCount > 20 || phase < 12) {
        return 0;
    }

    score = 0;
    undevelopedMinors = 0;
    homeRow = (color == WHITE) ? 7 : 0;

    for (col = 0; col < COLS; ++col) {
        Piece piece = getPiece(&state->board, createPosition(homeRow, col));

        if (piece.color != color) {
            continue;
        }

        // penalty for undeveloped knights and bishops
        if (piece.type == KNIGHT || piece.type == BISHOP) {
            ++undevelopedMinors;
            score -= 12;
        }
    }

    // penalty for queen when there are 2 or more undeveloped minors
    if (undevelopedMinors >= 2) {
        Piece queen = getPiece(&state->board, createPosition(homeRow, 4));

        if (queen.type != QUEEN || queen.color != color) {
            score -= 10;
        }
    }

    // bonus for castling
    if (getPiece(&state->board, createPosition(homeRow, 3)).type == KING &&
        getPiece(&state->board, createPosition(homeRow, 3)).color == color) {
        score += 8;
    }
    if (getPiece(&state->board, createPosition(homeRow, 7)).type == KING &&
        getPiece(&state->board, createPosition(homeRow, 7)).color == color) {
        score += 8;
    }

    return score;
}

static int evaluate_anteater_threats(const Board *board, Color color) {
    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = board->cells[row][col];
            int direction;

            if (piece.type != ANTEATER || piece.color != color) {
                continue;
            }

            for (direction = 0; direction < 4; ++direction) {
                int r2 = row + dr[direction];
                int c2 = col + dc[direction];
                int chainLength;

                chainLength = 0;
                while (r2 >= 0 && r2 < ROWS && c2 >= 0 && c2 < COLS) {
                    Piece target = board->cells[r2][c2];

                    if (target.type == ANT && target.color != color) {
                        ++chainLength;
                        r2 += dr[direction];
                        c2 += dc[direction];
                        continue;
                    }
                    break;
                }

                if (chainLength >= 2) {
                    score += 10 + chainLength * 15;
                }
            }
        }
    }

    return score;
}

static int evaluate_outposts(const Board *board, Color color, int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(board, pos);
            int bonus;

            if (piece.color != color) {
                continue;
            }
            if (piece.type != KNIGHT && piece.type != BISHOP && piece.type != ANTEATER) {
                continue;
            }
            if ((color == WHITE && row >= 5) || (color == BLACK && row <= 2)) {
                continue;
            }
            if (!ant_supports_square(board, color, pos)) {
                continue;
            }
            if (enemy_ant_can_attack_square(board, color, pos)) {
                continue;
            }

            if (piece.type == KNIGHT) {
                bonus = 16;
            } else if (piece.type == BISHOP) {
                bonus = 12;
            } else {
                bonus = 14;
            }

            // bonus for outpost in center
            if (col >= 3 && col <= 6) {
                bonus += 4;
            }

            // reduce score in end game
            if (phase <= 8) {
                bonus /= 2;
            }

            score += bonus;
        }
    }

    return score;
}

//**************************************************************** */
// evaluate the absolute score of the current board position
//**************************************************************** */
static int evaluate_absolute(const GameState *state) {
    int antFiles[2][COLS];
    int bishopCount[2];
    Position kingPos[2];
    int phase;
    int score;
    int row;
    int col;

    // init
    memset(antFiles, 0, sizeof(antFiles));
    memset(bishopCount, 0, sizeof(bishopCount));
    kingPos[WHITE] = createPosition(-1, -1);
    kingPos[BLACK] = createPosition(-1, -1);
    phase = 0;
    score = 0;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));

            if (piece.type == EMPTY_PIECE) {
                continue;
            }

            if (piece.type == ANT) {
                ++antFiles[piece.color][col];
            }
            if (piece.type == BISHOP) {
                ++bishopCount[piece.color];
            }
            if (piece.type == KING) {
                kingPos[piece.color] = createPosition(row, col);
            }

            phase += phase_value(piece.type);
        }
    }

    // robust
    if (phase > AI_MAX_PHASE) {
        phase = AI_MAX_PHASE;
    }

    // piece_value + PST + mobility
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(&state->board, pos);
            int value;

            if (piece.type == EMPTY_PIECE) {
                continue;
            }

            value = piece_value(piece.type);
            value += pst_bonus(piece, row, col, phase);
            if (piece.type == KNIGHT || piece.type == ANTEATER) {
                value += piece_mobility(&state->board, pos, piece) * mobility_weight(piece.type);
            }

            if (piece.color == WHITE) {
                score += value;
            } else {
                score -= value;
            }
        }
    }

    // bishop pair
    if (bishopCount[WHITE] >= 2) {
        score += 36;
    }
    if (bishopCount[BLACK] >= 2) {
        score -= 36;
    }

    score += evaluate_pawns(state, WHITE, antFiles, phase);
    score -= evaluate_pawns(state, BLACK, antFiles, phase);
    score += evaluate_rook_and_queen_files(state, WHITE, antFiles, phase);
    score -= evaluate_rook_and_queen_files(state, BLACK, antFiles, phase);
    score += evaluate_king_safety(state, WHITE, kingPos[WHITE], phase);
    score -= evaluate_king_safety(state, BLACK, kingPos[BLACK], phase);
    score += evaluate_development(state, WHITE, phase);
    score -= evaluate_development(state, BLACK, phase);
    score += evaluate_anteater_threats(&state->board, WHITE);
    score -= evaluate_anteater_threats(&state->board, BLACK);

    // outpost limitation
    if (phase >= 10) {
        score += evaluate_outposts(&state->board, WHITE, phase);
        score -= evaluate_outposts(&state->board, BLACK, phase);
    }

    return score;
}

// score for current player
static int evaluate_relative(const GameState *state) {
    int absoluteScore;

    absoluteScore = evaluate_absolute(state);
    // tempo bonus
    if (state->currentTurn == WHITE) {
        return absoluteScore + 1;
    }

    return -absoluteScore + 1;
}

static int elapsed_ms(const SearchContext *ctx) {
    int64_t now;

    if (ctx == NULL || getMonotonicMilliseconds(&now) != 0) {
        return INT_MAX;
    }

    if (now < ctx->searchStartMs) {
        return 0;
    }
    if (now - ctx->searchStartMs > INT_MAX) {
        return INT_MAX;
    }

    return (int)(now - ctx->searchStartMs);
}

static int time_is_up(SearchContext *ctx) {
    if (ctx->timeLimitMs <= 0) {
        return 0;
    }

    if ((ctx->nodes & 63) == 0 && elapsed_ms(ctx) >= ctx->timeLimitMs) {
        ctx->stopSearch = 1;
    }

    return ctx->stopSearch;
}

// reset and init
static void ensure_search_heuristics_ready(void) {
    if (g_searchHeuristicsReady != 0) {
        return;
    }

    memset(g_killerMoves, 0, sizeof(g_killerMoves));
    memset(g_killerValid, 0, sizeof(g_killerValid));
    memset(g_history, 0, sizeof(g_history));
    g_searchHeuristicsReady = 1;
}

// decrease all history score by 2, the knowledge from long-term search naturally decays
static void age_history_scores(void) {
    int color;
    int from;
    int to;

    ensure_search_heuristics_ready();
    for (color = 0; color < 2; ++color) {
        for (from = 0; from < ROWS * COLS; ++from) {
            for (to = 0; to < ROWS * COLS; ++to) {
                g_history[color][from][to] /= 2;
            }
        }
    }
}

static int init_search_context(SearchContext *ctx, int timeLimitMs) {
    if (ctx == NULL) {
        return 1;
    }

    ensure_search_heuristics_ready();
    memset(ctx, 0, sizeof(*ctx));
    ctx->moveBuffers = (MoveList *)calloc((size_t)(AI_MAX_PLY + 1), sizeof(MoveList));
    if (ctx->moveBuffers == NULL) {
        return 1;
    }

    if (getMonotonicMilliseconds(&ctx->searchStartMs) != 0) {
        ctx->searchStartMs = 0;
        ctx->stopSearch = 1;
    }
    ctx->timeLimitMs = timeLimitMs;
    ctx->softTimeLimitMs = (timeLimitMs > 0 && timeLimitMs <= INT_MAX / 4)
        ? (timeLimitMs * 4) / 5
        : timeLimitMs;
    ++g_ttGeneration;
    if (g_ttGeneration == 0) {
        ++g_ttGeneration;
    }
    ctx->generation = g_ttGeneration;
    return 0;
}

static void destroy_search_context(SearchContext *ctx) {
    if (ctx != NULL) {
        free(ctx->moveBuffers);
        ctx->moveBuffers = NULL;
    }
}

static int build_game_hash_history(SearchContext *ctx, const GameState *state) {
    GameState replay;
    HashState currentHash;
    HashState nextHash;
    int moveIndex;

    if (ctx == NULL || state == NULL) {
        return 1;
    }

    initZobrist();
    initGameState(&replay, &state->config);
    if (deriveHashState(&replay, &currentHash) != 0) {
        return 1;
    }

    ctx->gameHashes[0] = currentHash.value;
    ctx->gameHashCount = 1;
    ctx->gameHistoryStart = 0;
    for (moveIndex = 0; moveIndex < state->moveHistory.count; ++moveIndex) {
        Move *move = getMove((MoveList *)&state->moveHistory, moveIndex);

        if (move == NULL) {
            return 1;
        }
        // reset game history start, because of irreversible move
        if (is_irreversible_move(move)) {
            ctx->gameHistoryStart = ctx->gameHashCount;
        }
        if (applyMove(&replay, *move) != 0) {
            return 1;
        }
        if (advanceHashState(&replay, *move, &currentHash, &nextHash) != 0) {
            return 1;
        }

        currentHash = nextHash;
        ctx->gameHashes[ctx->gameHashCount++] = currentHash.value;
    }

    return 0;
}

static int node_is_repetition(const SearchContext *ctx, uint64_t key, int ply) {
    int index;

    if (ctx == NULL || ctx->nullMoveActive[ply]) {
        return 0;
    }

    for (index = ply - 1; index >= ctx->repetitionLimit[ply]; --index) {
        if (ctx->hashStack[index].value == key) {
            return 1;
        }
    }
    if (ctx->repetitionLimit[ply] == 0) {
        for (index = ctx->gameHistoryStart; index + 1 < ctx->gameHashCount; ++index) {
            if (ctx->gameHashes[index] == key) {
                return 1;
            }
        }
    }

    return 0;
}

static TTEntry *tt_slot(uint64_t key) {
    return &g_transpositionTable[key & (TT_SIZE - 1)];
}

static int score_to_tt(int score, int ply) {
    if (score > AI_MATE - 1000) {
        return score + ply;
    }
    if (score < -AI_MATE + 1000) {
        return score - ply;
    }

    return score;
}

static int score_from_tt(int score, int ply) {
    if (score > AI_MATE - 1000) {
        return score - ply;
    }
    if (score < -AI_MATE + 1000) {
        return score + ply;
    }

    return score;
}

static int tt_lookup(uint64_t key, TTEntry *entry) {
    TTEntry *slot;

    slot = tt_slot(key);
    if (slot->key != key) {
        return 0;
    }

    if (entry != NULL) {
        *entry = *slot;
    }
    return 1;
}

static void tt_store(uint64_t key, int depth, int ply, int score, int flag, const Move *bestMove,
                     unsigned char generation) {
    TTEntry *entry;
    int replace;

    entry = tt_slot(key);
    replace = 0;
    if (entry->key != key) {
        replace = 1;
    } else if (flag == TT_FLAG_EXACT && entry->flag != TT_FLAG_EXACT) {
        replace = 1;
    } else if (depth >= entry->depth) {
        replace = 1;
    } else if (entry->generation != generation) {
        replace = 1;
    }

    if (!replace) {
        return;
    }

    entry->key = key;
    entry->score = score_to_tt(score, ply);
    entry->depth = (short)depth;
    entry->flag = (unsigned char)flag;
    entry->generation = generation;
    if (bestMove != NULL) {
        entry->from = (unsigned char)square_index(bestMove->from);
        entry->to = (unsigned char)square_index(bestMove->to);
        entry->special = (unsigned char)bestMove->specialType;
    } else {
        entry->from = 255;
        entry->to = 255;
        entry->special = (unsigned char)NO_SPECIAL_MOVE;
    }
}

static int move_matches_entry(const Move *move, const TTEntry *entry) {
    if (move == NULL || entry == NULL || entry->from >= ROWS * COLS || entry->to >= ROWS * COLS) {
        return 0;
    }

    return square_index(move->from) == entry->from && square_index(move->to) == entry->to &&
           move->specialType == (SpecialMove)entry->special;
}

static int move_equal_signature(const Move *lhs, const Move *rhs) {
    if (lhs == NULL || rhs == NULL) {
        return 0;
    }

    return positionEqual(lhs->from, rhs->from) && positionEqual(lhs->to, rhs->to) &&
           lhs->specialType == rhs->specialType;
}

static void save_killer(SearchContext *ctx, int ply, Move move) {
    (void)ctx;
    ensure_search_heuristics_ready();
    if (ply >= AI_MAX_PLY || !is_quiet_move(&move)) {
        return;
    }

    if (g_killerValid[ply][0] && move_equal_signature(&g_killerMoves[ply][0], &move)) {
        return;
    }

    if (g_killerValid[ply][0]) {
        g_killerMoves[ply][1] = g_killerMoves[ply][0];
        g_killerValid[ply][1] = 1;
    }
    g_killerMoves[ply][0] = move;
    g_killerValid[ply][0] = 1;
}

static void update_history_score(SearchContext *ctx, Color color, Move move, int delta) {
    int from;
    int to;
    int *cell;

    (void)ctx;
    ensure_search_heuristics_ready();
    if (!is_quiet_move(&move)) {
        return;
    }

    from = square_index(move.from);
    to = square_index(move.to);
    cell = &g_history[color][from][to];
    *cell += delta;
    if (*cell > AI_HISTORY_MAX) {
        *cell = AI_HISTORY_MAX;
    } else if (*cell < -AI_HISTORY_MAX) {
        *cell = -AI_HISTORY_MAX;
    }
}

static int tactical_move_score(const Move *move) {
    int score;
    int captureIndex;

    score = 0;
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        score += piece_value(move->captures[captureIndex].piece.type) * 16;
    }

    score -= piece_value(move->movedPiece.type);
    if (move->specialType == ANTEATER_CAPTURE) {
        score += 300 + move->captureCount * 120;
    }
    if (is_promotion_move(move)) {
        score += 850;
    }

    return score;
}

static int quick_exchange_margin(const Move *move) {
    int gain;
    int risk;

    if (move == NULL) {
        return 0;
    }

    gain = see_initial_gain(move);
    risk = piece_value(move->movedPiece.type);
    if (move->specialType == ANTEATER_CAPTURE && move->captureCount >= 2) {
        risk /= 2;
    }

    return gain - risk;
}

static int move_order_score(SearchContext *ctx, const GameState *state, const Move *move, int ply,
                            const TTEntry *ttMove) {
    int from;
    int to;
    int score;

    (void)ctx;
    ensure_search_heuristics_ready();
    if (move_matches_entry(move, ttMove)) {
        return INT_MAX;
    }
    if (is_noisy_move(move)) {
        int quickMargin = quick_exchange_margin(move);
        int seeScore = quickMargin;
        int tacticalScore = tactical_move_score(move);

        if (!is_promotion_move(move) && quickMargin > -250 && quickMargin < 250) {
            seeScore = see_move_score(state, move);
        }
        if (seeScore >= 0 || is_promotion_move(move)) {
            return 1000000 + seeScore * 64 + tacticalScore;
        }
        return 220000 + seeScore * 64 + tacticalScore;
    }
    if (ply < AI_MAX_PLY && g_killerValid[ply][0] && move_equal_signature(move, &g_killerMoves[ply][0])) {
        return 900000;
    }
    if (ply < AI_MAX_PLY && g_killerValid[ply][1] && move_equal_signature(move, &g_killerMoves[ply][1])) {
        return 899000;
    }

    from = square_index(move->from);
    to = square_index(move->to);
    score = g_history[state->currentTurn][from][to];
    if (move->specialType == CASTLING_KINGSIDE || move->specialType == CASTLING_QUEENSIDE) {
        score += 150;
    }

    return score;
}

static void sort_moves(SearchContext *ctx, const GameState *state, MoveList *list, int ply, const TTEntry *ttMove) {
    int scores[MAX_MOVES];
    int index;

    for (index = 0; index < list->count; ++index) {
        scores[index] = move_order_score(ctx, state, &list->moves[index], ply, ttMove);
    }

    for (index = 1; index < list->count; ++index) {
        Move keyMove = list->moves[index];
        int keyScore = scores[index];
        int scan = index - 1;

        while (scan >= 0 && scores[scan] < keyScore) {
            list->moves[scan + 1] = list->moves[scan];
            scores[scan + 1] = scores[scan];
            --scan;
        }

        list->moves[scan + 1] = keyMove;
        scores[scan + 1] = keyScore;
    }
}

static void sort_root_moves_by_scores(MoveList *list, int scores[MAX_MOVES]) {
    int index;

    for (index = 1; index < list->count; ++index) {
        Move keyMove = list->moves[index];
        int keyScore = scores[index];
        int scan = index - 1;

        while (scan >= 0 && scores[scan] < keyScore) {
            list->moves[scan + 1] = list->moves[scan];
            scores[scan + 1] = scores[scan];
            --scan;
        }

        list->moves[scan + 1] = keyMove;
        scores[scan + 1] = keyScore;
    }
}

static int generate_search_moves(GameState *state, MoveList *list, int onlyNoisy) {
    int index;
    int writeIndex;
    int total;

    if (state == NULL || list == NULL) {
        return 1;
    }

    if (state->moveHistory.count > MAX_MOVES - AI_MAX_PLY - 2) {
        if (generateLegalMoves(state, list) != 0) {
            return 1;
        }
        if (onlyNoisy) {
            total = list->count;
            writeIndex = 0;
            for (index = 0; index < total; ++index) {
                if (is_noisy_move(&list->moves[index])) {
                    list->moves[writeIndex++] = list->moves[index];
                }
            }
            list->count = writeIndex;
        }
        return 0;
    }

    if (generateMoves(state, list) != 0) {
        return 1;
    }

    if (!onlyNoisy) {
        return 0;
    }

    total = list->count;
    writeIndex = 0;
    for (index = 0; index < total; ++index) {
        if (is_noisy_move(&list->moves[index])) {
            list->moves[writeIndex++] = list->moves[index];
        }
    }
    list->count = writeIndex;
    return 0;
}

static int side_has_major_material(const Board *board, Color color) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = board->cells[row][col];

            if (piece.color != color) {
                continue;
            }
            switch (piece.type) {
            case KNIGHT:
            case BISHOP:
            case ROOK:
            case QUEEN:
            case ANTEATER:
                return 1;
            case ANT:
            case KING:
            case EMPTY_PIECE:
            default:
                break;
            }
        }
    }

    return 0;
}

static int quiescence(SearchContext *ctx, GameState *state, int alpha, int beta, int ply, int qDepth);

static int alpha_beta(SearchContext *ctx, GameState *state, int depth, int alpha, int beta, int ply, int allowNull);

static int try_null_move(SearchContext *ctx, GameState *state, int depth, int beta, int ply) {
    Color originalTurn;
    int originalHistoryCount;
    int originalMoveCount;
    Move dummy;
    int score;
    int reduction;

    if (state == NULL || ply + 1 >= AI_MAX_PLY || state->moveHistory.count >= MAX_MOVES) {
        return beta - 1;
    }

    originalTurn = state->currentTurn;
    originalHistoryCount = state->moveHistory.count;
    originalMoveCount = state->moveCount;
    dummy = createMove(createPosition(-1, -1), createPosition(-1, -1), createPiece(EMPTY_PIECE, EMPTY_COLOR));

    state->currentTurn = (originalTurn == WHITE) ? BLACK : WHITE;
    state->moveHistory.moves[state->moveHistory.count++] = dummy;
    state->moveCount += 1;
    if (deriveHashState(state, &ctx->hashStack[ply + 1]) != 0) {
        state->currentTurn = originalTurn;
        state->moveHistory.count = originalHistoryCount;
        state->moveCount = originalMoveCount;
        return beta - 1;
    }
    ctx->repetitionLimit[ply + 1] = ply + 1;
    ctx->nullMoveActive[ply + 1] = 1;

    reduction = NULL_MOVE_R;
    if (depth >= 6) {
        ++reduction;
    }
    score = -alpha_beta(ctx, state, depth - 1 - reduction, -beta, -beta + 1, ply + 1, 0);

    state->currentTurn = originalTurn;
    state->moveHistory.count = originalHistoryCount;
    state->moveCount = originalMoveCount;
    return score;
}

static int alpha_beta(SearchContext *ctx, GameState *state, int depth, int alpha, int beta, int ply, int allowNull) {
    TTEntry ttEntry;
    int ttHit;
    int originalAlpha;
    int bestScore;
    int inCheck;
    int index;
    int bestMoveValid;
    int legalCount;
    Move bestMove;
    MoveList *moves;
    uint64_t key;
    int pvNode;
    Color movingSide;

    if (time_is_up(ctx)) {
        return evaluate_relative(state);
    }

    if (ply >= AI_MAX_PLY - 2) {
        return evaluate_relative(state);
    }

    ++ctx->nodes;
    key = ctx->hashStack[ply].value;
    if (node_is_repetition(ctx, key, ply)) {
        return 0;
    }
    if (alpha < -AI_MATE + ply) {
        alpha = -AI_MATE + ply;
    }
    if (beta > AI_MATE - ply - 1) {
        beta = AI_MATE - ply - 1;
    }
    if (alpha >= beta) {
        return alpha;
    }
    ttHit = tt_lookup(key, &ttEntry);
    pvNode = (beta - alpha > 1);
    if (ttHit && ttEntry.depth >= depth) {
        int ttScore = score_from_tt(ttEntry.score, ply);

        if (ttEntry.flag == TT_FLAG_EXACT) {
            return ttScore;
        }
        if (!pvNode && ttEntry.flag == TT_FLAG_LOWER && ttScore >= beta) {
            return ttScore;
        }
        if (!pvNode && ttEntry.flag == TT_FLAG_UPPER && ttScore <= alpha) {
            return ttScore;
        }
    }

    inCheck = isInCheck(state, state->currentTurn);
    if (inCheck) {
        ++depth;
    }
    if (depth <= 0) {
        return quiescence(ctx, state, alpha, beta, ply, 0);
    }

    if (allowNull && !pvNode && !inCheck && depth >= 3 && side_has_major_material(&state->board, state->currentTurn)) {
        int nullScore = try_null_move(ctx, state, depth, beta, ply);

        if (ctx->stopSearch) {
            return alpha;
        }
        if (nullScore >= beta) {
            if (nullScore >= AI_MATE - 1000) {
                nullScore = beta;
            }
            return nullScore;
        }
    }

    moves = &ctx->moveBuffers[ply];
    if (generate_search_moves(state, moves, 0) != 0) {
        return evaluate_relative(state);
    }
    if (moves->count == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return 0;
    }

    sort_moves(ctx, state, moves, ply, ttHit ? &ttEntry : NULL);
    originalAlpha = alpha;
    bestScore = -AI_INF;
    bestMoveValid = 0;
    legalCount = 0;
    movingSide = state->currentTurn;

    for (index = 0; index < moves->count; ++index) {
        Move move = moves->moves[index];
        int score;
        int childDepth;
        int reduction;
        int extension;

        if (applyMove(state, move) != 0) {
            continue;
        }
        if (isInCheck(state, movingSide) != 0) {
            if (undoMove(state) != 0) {
                return evaluate_relative(state);
            }
            continue;
        }

        ++legalCount;
        if (advanceHashState(state, move, &ctx->hashStack[ply], &ctx->hashStack[ply + 1]) != 0) {
            if (undoMove(state) != 0) {
                return evaluate_relative(state);
            }
            continue;
        }
        ctx->repetitionLimit[ply + 1] = is_irreversible_move(&move) ? (ply + 1) : ctx->repetitionLimit[ply];
        ctx->nullMoveActive[ply + 1] = ctx->nullMoveActive[ply];

        extension = 0;
        if (depth <= 6) {
            if (is_promotion_move(&move)) {
                extension = 1;
            } else if (move.specialType == ANTEATER_CAPTURE && move.captureCount >= 2) {
                extension = 1;
            }
        }
        childDepth = depth - 1 + extension;

        reduction = 0;
        if (legalCount >= 4 && depth >= 4 && !inCheck && is_quiet_move(&move) && !pvNode) {
            reduction = 1;
            if (legalCount >= 8 && depth >= 5) {
                ++reduction;
            }
            if (legalCount >= 12 && depth >= 8) {
                ++reduction;
            }
            if (reduction > childDepth - 1) {
                reduction = childDepth - 1;
            }
            if (reduction < 0) {
                reduction = 0;
            }
        }

        if (legalCount == 1) {
            score = -alpha_beta(ctx, state, childDepth, -beta, -alpha, ply + 1, 1);
        } else {
            int reducedDepth = childDepth - reduction;

            if (reducedDepth < 0) {
                reducedDepth = 0;
            }
            score = -alpha_beta(ctx, state, reducedDepth, -alpha - 1, -alpha, ply + 1, 1);
            if (!ctx->stopSearch && reduction > 0 && score > alpha) {
                score = -alpha_beta(ctx, state, childDepth, -alpha - 1, -alpha, ply + 1, 1);
            }
            if (!ctx->stopSearch && score > alpha && score < beta) {
                score = -alpha_beta(ctx, state, childDepth, -beta, -alpha, ply + 1, 1);
            }
        }

        if (undoMove(state) != 0) {
            return evaluate_relative(state);
        }
        if (ctx->stopSearch) {
            return alpha;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            bestMoveValid = 1;
        }
        if (score > alpha) {
            alpha = score;
        }
        if (alpha >= beta) {
            if (is_quiet_move(&move)) {
                save_killer(ctx, ply, move);
                update_history_score(ctx, movingSide, move, depth * depth + 8);
            }
            tt_store(key, depth, ply, alpha, TT_FLAG_LOWER, &move, ctx->generation);
            return alpha;
        }
        if (is_quiet_move(&move)) {
            update_history_score(ctx, movingSide, move, -(depth + 1));
        }
    }

    if (legalCount == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return 0;
    }
    if (!bestMoveValid) {
        return (inCheck != 0) ? (-AI_MATE + ply) : 0;
    }

    tt_store(key, depth, ply, bestScore, (bestScore <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, &bestMove,
             ctx->generation);
    return bestScore;
}

static int quiescence(SearchContext *ctx, GameState *state, int alpha, int beta, int ply, int qDepth) {
    TTEntry ttEntry;
    int ttHit;
    int originalAlpha;
    int standPat;
    int inCheck;
    int index;
    int legalCount;
    MoveList *moves;
    uint64_t key;
    Color movingSide;

    if (time_is_up(ctx)) {
        return evaluate_relative(state);
    }
    if (ply >= AI_MAX_PLY - 2) {
        return evaluate_relative(state);
    }

    ++ctx->nodes;
    originalAlpha = alpha;
    key = ctx->hashStack[ply].value;
    if (node_is_repetition(ctx, key, ply)) {
        return 0;
    }
    ttHit = tt_lookup(key, &ttEntry);
    if (ttHit && ttEntry.depth >= 0) {
        int ttScore = score_from_tt(ttEntry.score, ply);

        if (ttEntry.flag == TT_FLAG_EXACT) {
            return ttScore;
        }
        if (ttEntry.flag == TT_FLAG_LOWER && ttScore >= beta) {
            return ttScore;
        }
        if (ttEntry.flag == TT_FLAG_UPPER && ttScore <= alpha) {
            return ttScore;
        }
    }

    inCheck = isInCheck(state, state->currentTurn);
    standPat = alpha;
    if (!inCheck) {
        standPat = evaluate_relative(state);
        if (standPat >= beta) {
            tt_store(key, 0, ply, standPat, TT_FLAG_LOWER, NULL, ctx->generation);
            return standPat;
        }
        if (standPat > alpha) {
            alpha = standPat;
        }
        if (qDepth >= AI_Q_DEPTH) {
            tt_store(key, 0, ply, alpha, (alpha <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, NULL,
                     ctx->generation);
            return alpha;
        }
    } else if (qDepth >= AI_Q_DEPTH + 2) {
        return evaluate_relative(state);
    }

    moves = &ctx->moveBuffers[ply];
    if (generate_search_moves(state, moves, !inCheck) != 0) {
        return alpha;
    }
    if (moves->count == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return alpha;
    }

    sort_moves(ctx, state, moves, ply, ttHit ? &ttEntry : NULL);
    legalCount = 0;
    movingSide = state->currentTurn;
    for (index = 0; index < moves->count; ++index) {
        Move move = moves->moves[index];
        int score;

        if (!inCheck) {
            int quickMargin;

            quickMargin = quick_exchange_margin(&move);
            if (quickMargin < 0 && !is_promotion_move(&move)) {
                int seeScore = see_move_score(state, &move);

                if (seeScore < 0) {
                    continue;
                }
                quickMargin = seeScore;
            }
            if (standPat + see_initial_gain(&move) + 100 < alpha && !is_promotion_move(&move)) {
                continue;
            }
            (void)quickMargin;
        }

        if (applyMove(state, move) != 0) {
            continue;
        }
        if (isInCheck(state, movingSide) != 0) {
            if (undoMove(state) != 0) {
                return alpha;
            }
            continue;
        }

        ++legalCount;
        if (advanceHashState(state, move, &ctx->hashStack[ply], &ctx->hashStack[ply + 1]) != 0) {
            if (undoMove(state) != 0) {
                return alpha;
            }
            continue;
        }
        ctx->repetitionLimit[ply + 1] = is_irreversible_move(&move) ? (ply + 1) : ctx->repetitionLimit[ply];
        ctx->nullMoveActive[ply + 1] = ctx->nullMoveActive[ply];

        score = -quiescence(ctx, state, -beta, -alpha, ply + 1, qDepth + 1);
        if (undoMove(state) != 0) {
            return alpha;
        }
        if (ctx->stopSearch) {
            return alpha;
        }
        if (score >= beta) {
            tt_store(key, 0, ply, score, TT_FLAG_LOWER, &move, ctx->generation);
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (legalCount == 0 && inCheck) {
        return -AI_MATE + ply;
    }

    tt_store(key, 0, ply, alpha, (alpha <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, NULL, ctx->generation);
    return alpha;
}

static AIDifficulty difficulty_for_turn(const GameState *state) {
    AIDifficulty difficulty;

    if (state->currentTurn == WHITE) {
        difficulty = state->config.aiDifficultyWhite;
    } else {
        difficulty = state->config.aiDifficultyBlack;
    }

    if (difficulty == DIFFICULTY_NONE) {
        difficulty = DIFFICULTY_MEDIUM;
    }
    return difficulty;
}

static int depth_for_difficulty(AIDifficulty difficulty) {
    switch (difficulty) {
    case DIFFICULTY_EASY:
        return 2;
    case DIFFICULTY_MEDIUM:
        return 10;
    case DIFFICULTY_HARD:
        return 24;
    case DIFFICULTY_NONE:
    default:
        return 8;
    }
}

static int time_budget_for_state(const GameState *state, AIDifficulty difficulty) {
    int budget;

    budget = getAITimeBudgetMs((state != NULL) ? &state->config : NULL, difficulty);
    if (budget <= 0) {
        return getDefaultAITimeBudgetMs(DIFFICULTY_MEDIUM);
    }

    return budget;
}

static int search_best_move(const GameState *state, int maxDepth, int maxTimeMs, Move *bestMove) {
    SearchContext ctx;
    GameState searchState;
    MoveList *rootMoves;
    TTEntry rootEntry;
    int rootScores[MAX_MOVES];
    Move currentBest;
    int currentBestScore;
    int depth;

    if (state == NULL || bestMove == NULL) {
        return 1;
    }

    if (maxDepth > AI_MAX_PLY - 2) {
        maxDepth = AI_MAX_PLY - 2;
    }
    if (init_search_context(&ctx, maxTimeMs) != 0) {
        return 1;
    }
    age_history_scores();

    initZobrist();
    searchState = *state;
    if (deriveHashState(&searchState, &ctx.hashStack[0]) != 0) {
        destroy_search_context(&ctx);
        return 1;
    }
    if (build_game_hash_history(&ctx, &searchState) != 0) {
        ctx.gameHashes[0] = ctx.hashStack[0].value;
        ctx.gameHashCount = 1;
        ctx.gameHistoryStart = 0;
    }
    ctx.repetitionLimit[0] = 0;
    ctx.nullMoveActive[0] = 0;

    rootMoves = &ctx.moveBuffers[0];
    if (generateLegalMoves(&searchState, rootMoves) != 0 || rootMoves->count <= 0) {
        destroy_search_context(&ctx);
        return 1;
    }

    memset(rootScores, 0, sizeof(rootScores));
    if (tt_lookup(ctx.hashStack[0].value, &rootEntry)) {
        sort_moves(&ctx, &searchState, rootMoves, 0, &rootEntry);
    } else {
        sort_moves(&ctx, &searchState, rootMoves, 0, NULL);
    }

    *bestMove = rootMoves->moves[0];
    if (rootMoves->count == 1) {
        destroy_search_context(&ctx);
        return 0;
    }

    currentBest = rootMoves->moves[0];
    currentBestScore = -AI_INF;

    for (depth = 1; depth <= maxDepth; ++depth) {
        Move iterationBest;
        int iterationBestScore;
        int aspiration;
        int alphaBase;
        int betaBase;
        int attempt;

        if (time_is_up(&ctx)) {
            break;
        }

        iterationBest = currentBest;
        iterationBestScore = -AI_INF;
        aspiration = ASPIRATION_WINDOW;
        if (depth > 1 && currentBestScore > -AI_INF / 2) {
            alphaBase = currentBestScore - aspiration;
            betaBase = currentBestScore + aspiration;
        } else {
            alphaBase = -AI_INF;
            betaBase = AI_INF;
        }

        for (attempt = 0; attempt < 4; ++attempt) {
            int alpha;
            int beta;
            int localAlpha;
            int index;

            alpha = alphaBase;
            beta = betaBase;
            localAlpha = alpha;
            iterationBest = currentBest;
            iterationBestScore = -AI_INF;

            for (index = 0; index < rootMoves->count; ++index) {
                Move move = rootMoves->moves[index];
                int score;

                if (time_is_up(&ctx)) {
                    break;
                }

                if (applyMove(&searchState, move) != 0) {
                    rootScores[index] = -AI_INF;
                    continue;
                }
                if (advanceHashState(&searchState, move, &ctx.hashStack[0], &ctx.hashStack[1]) != 0) {
                    if (undoMove(&searchState) != 0) {
                        destroy_search_context(&ctx);
                        return 1;
                    }
                    rootScores[index] = -AI_INF;
                    continue;
                }
                ctx.repetitionLimit[1] = is_irreversible_move(&move) ? 1 : 0;
                ctx.nullMoveActive[1] = 0;

                if (index == 0) {
                    score = -alpha_beta(&ctx, &searchState, depth - 1, -beta, -localAlpha, 1, 1);
                } else {
                    score = -alpha_beta(&ctx, &searchState, depth - 1, -localAlpha - 1, -localAlpha, 1, 1);
                    if (!ctx.stopSearch && score > localAlpha && score < beta) {
                        score = -alpha_beta(&ctx, &searchState, depth - 1, -beta, -localAlpha, 1, 1);
                    }
                }

                if (undoMove(&searchState) != 0) {
                    destroy_search_context(&ctx);
                    return 1;
                }
                if (ctx.stopSearch) {
                    break;
                }

                rootScores[index] = score;
                if (score > iterationBestScore) {
                    iterationBestScore = score;
                    iterationBest = move;
                }
                if (score > localAlpha) {
                    localAlpha = score;
                }
            }

            if (ctx.stopSearch) {
                break;
            }

            if (alphaBase != -AI_INF || betaBase != AI_INF) {
                if (iterationBestScore <= alpha) {
                    alphaBase -= aspiration * 4;
                    if (alphaBase < -AI_INF) {
                        alphaBase = -AI_INF;
                    }
                    aspiration *= 2;
                    continue;
                }
                if (iterationBestScore >= beta) {
                    betaBase += aspiration * 4;
                    if (betaBase > AI_INF) {
                        betaBase = AI_INF;
                    }
                    aspiration *= 2;
                    continue;
                }
            }
            break;
        }

        if (ctx.stopSearch) {
            break;
        }

        currentBest = iterationBest;
        currentBestScore = iterationBestScore;
        *bestMove = currentBest;
        sort_root_moves_by_scores(rootMoves, rootScores);
        tt_store(ctx.hashStack[0].value, depth, 0, currentBestScore, TT_FLAG_EXACT, &currentBest, ctx.generation);

        if (currentBestScore >= AI_MATE - 1000 || currentBestScore <= -AI_MATE + 1000) {
            break;
        }
        if (ctx.softTimeLimitMs > 0 && elapsed_ms(&ctx) >= ctx.softTimeLimitMs) {
            break;
        }
    }

    destroy_search_context(&ctx);
    return 0;
}

int generateAIMove(const GameState *state, Move *move) {
    AIDifficulty difficulty;
    AIDifficulty effective;
    int maxDepth;
    int maxTimeMs;

    if (state == NULL || move == NULL) {
        return 1;
    }

    difficulty = difficulty_for_turn(state);

    if (difficulty == DIFFICULTY_EXPERIMENTAL) {
#if HAS_ALIEN_PLUGIN
        int budget = time_budget_for_state(state, DIFFICULTY_HARD);
        if (alien_plugin_generate_move(state, move, budget) == 0) {
            return 0;
        }
        /* Plugin failed — degrade to HARD instead of forfeiting. */
#endif
        effective = DIFFICULTY_HARD;
    } else {
        effective = difficulty;
    }

    maxDepth = depth_for_difficulty(effective);
    maxTimeMs = time_budget_for_state(state, effective);
    if (search_best_move(state, maxDepth, maxTimeMs, move) != 0) {
        return 1;
    }
    return 0;
}

int generateHintMove(const GameState *state, Move *move) {
    int maxTimeMs;

    if (state == NULL || move == NULL) {
        return 1;
    }

    maxTimeMs = time_budget_for_state(state, DIFFICULTY_MEDIUM);
    return search_best_move(state, 8, maxTimeMs, move);
}
