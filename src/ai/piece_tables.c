#include "anteater/rules.h"
#include "score_constants.h"
#include "piece_tables.h"

/*
******************************
AcPiece-Square Table (PST)
******************************
*/

static const int PST_ANT[AC_ROWS][AC_COLS] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  0,  0,  5,  5,  5,  5,  0,  0,  0 },
    {5,  5,  10, 15, 20, 20, 15, 10, 5,  5 },
    {5,  10, 15, 25, 30, 30, 25, 15, 10, 5 },
    {10, 15, 25, 35, 40, 40, 35, 25, 15, 10},
    {20, 25, 35, 45, 50, 50, 45, 35, 25, 20},
    {50, 50, 50, 50, 50, 50, 50, 50, 50, 50},
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

static const int PST_KNIGHT[AC_ROWS][AC_COLS] = {
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50}
};

static const int PST_BISHOP[AC_ROWS][AC_COLS] = {
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20},
    {-10, 10,  0,   0,   5,   5,   0,   0,   10,  -10},
    {-10, 10,  10,  10,  10,  10,  10,  10,  10,  -10},
    {-10, 0,   15,  20,  20,  20,  20,  15,  0,   -10},
    {-10, 5,   15,  20,  20,  20,  20,  15,  5,   -10},
    {-10, 0,   10,  15,  15,  15,  15,  10,  0,   -10},
    {-10, 5,   0,   0,   0,   0,   0,   0,   5,   -10},
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20}
};

static const int PST_ROOK[AC_ROWS][AC_COLS] = {
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 },
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {10, 15, 15, 15, 15, 15, 15, 15, 15, 10},
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 }
};

static const int PST_QUEEN[AC_ROWS][AC_COLS] = {
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20},
    {-10, 0,   5,   0,  0,  0,  0,  5,   0,   -10},
    {-10, 5,   5,   5,  5,  5,  5,  5,   5,   -10},
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-10, 0,   5,   5,  5,  5,  5,  5,   0,   -10},
    {-10, 0,   0,   0,  0,  0,  0,  0,   0,   -10},
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20}
};

static const int PST_KING_MID[AC_ROWS][AC_COLS] = {
    {20,  30,  10,  0,   0,   0,   0,   10,  30,  20 },
    {20,  20,  0,   -5,  -5,  -5,  -5,  0,   20,  20 },
    {-10, -20, -20, -20, -20, -20, -20, -20, -20, -10},
    {-20, -30, -30, -40, -40, -40, -40, -30, -30, -20},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30}
};

static const int PST_KING_END[AC_ROWS][AC_COLS] = {
    {-50, -40, -30, -20, -20, -20, -20, -30, -40, -50},
    {-30, -15, -5,  5,   5,   5,   5,   -5,  -15, -30},
    {-20, -5,  10,  15,  20,  20,  15,  10,  -5,  -20},
    {-10, 0,   15,  25,  30,  30,  25,  15,  0,   -10},
    {-10, 0,   15,  25,  30,  30,  25,  15,  0,   -10},
    {-20, -5,  10,  15,  20,  20,  15,  10,  -5,  -20},
    {-30, -15, -5,  5,   5,   5,   5,   -5,  -15, -30},
    {-50, -40, -30, -20, -20, -20, -20, -30, -40, -50}
};

static const int PST_ANTEATER[AC_ROWS][AC_COLS] = {
    {15, 15, 15, 15, 15, 15, 15, 15, 15, 15},
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {15, 15, 15, 15, 15, 15, 15, 15, 15, 15}
};

// AcPiece Value Tables
int ac_ai_piece_value(AcPieceType type) {
    switch (type) {
    case AC_ANT:
        return 100;
    case AC_KNIGHT:
        return 320;
    case AC_BISHOP:
        return 335;
    case AC_ROOK:
        return 510;
    case AC_QUEEN:
        return 950;
    case AC_KING:
        return 20000;
    case AC_ANTEATER:
        return 235;
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

int ac_ai_phase_value(AcPieceType type) {
    switch (type) {
    case AC_KNIGHT:
        return 1;
    case AC_BISHOP:
        return 1;
    case AC_ROOK:
        return 2;
    case AC_QUEEN:
        return 4;
    case AC_ANTEATER:
        return 1;
    case AC_ANT:
    case AC_KING:
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

int ac_ai_mobility_weight(AcPieceType type) {
    switch (type) {
    case AC_ANT:
        return 4;
    case AC_KNIGHT:
        return 5;
    case AC_BISHOP:
        return 5;
    case AC_ROOK:
        return 3;
    case AC_QUEEN:
        return 2;
    case AC_ANTEATER:
        return 6;
    case AC_KING:
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

static int ac_ai_board_row_for_pst(AcColor color, int row) {
    return (color == AC_WHITE) ? (AC_ROWS - 1 - row) : row;
}

static int ac_ai_king_pst_bonus(AcPiece piece, int row, int col, int phase) {
    int pstRow;
    int midScore;
    int endScore;

    pstRow = ac_ai_board_row_for_pst(piece.color, row);
    midScore = PST_KING_MID[pstRow][col];
    endScore = PST_KING_END[pstRow][col];
    // Phase AI_MAX_PHASE selects the opening table; phase zero selects the endgame.
    return (midScore * phase + endScore * (AI_MAX_PHASE - phase)) / AI_MAX_PHASE;
}

int ac_ai_pst_bonus(AcPiece piece, int row, int col, int phase) {
    int pstRow;

    pstRow = ac_ai_board_row_for_pst(piece.color, row);
    switch (piece.type) {
    case AC_ANT:
        return PST_ANT[pstRow][col];
    case AC_KNIGHT:
        return PST_KNIGHT[pstRow][col];
    case AC_BISHOP:
        return PST_BISHOP[pstRow][col];
    case AC_ROOK:
        return PST_ROOK[pstRow][col];
    case AC_QUEEN:
        return PST_QUEEN[pstRow][col];
    case AC_KING:
        return ac_ai_king_pst_bonus(piece, row, col, phase);
    case AC_ANTEATER:
        return PST_ANTEATER[pstRow][col];
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

