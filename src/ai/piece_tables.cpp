#include "anteater/rules.hpp"
#include "score_constants.hpp"
#include "piece_tables.hpp"

namespace ac {

/*
******************************
Piece-Square Table (PST)
******************************
*/

static const int PST_ANT[Rows][Columns] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  0,  0,  5,  5,  5,  5,  0,  0,  0 },
    {5,  5,  10, 15, 20, 20, 15, 10, 5,  5 },
    {5,  10, 15, 25, 30, 30, 25, 15, 10, 5 },
    {10, 15, 25, 35, 40, 40, 35, 25, 15, 10},
    {20, 25, 35, 45, 50, 50, 45, 35, 25, 20},
    {50, 50, 50, 50, 50, 50, 50, 50, 50, 50},
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

static const int PST_KNIGHT[Rows][Columns] = {
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50}
};

static const int PST_BISHOP[Rows][Columns] = {
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20},
    {-10, 10,  0,   0,   5,   5,   0,   0,   10,  -10},
    {-10, 10,  10,  10,  10,  10,  10,  10,  10,  -10},
    {-10, 0,   15,  20,  20,  20,  20,  15,  0,   -10},
    {-10, 5,   15,  20,  20,  20,  20,  15,  5,   -10},
    {-10, 0,   10,  15,  15,  15,  15,  10,  0,   -10},
    {-10, 5,   0,   0,   0,   0,   0,   0,   5,   -10},
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20}
};

static const int PST_ROOK[Rows][Columns] = {
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 },
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {10, 15, 15, 15, 15, 15, 15, 15, 15, 10},
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 }
};

static const int PST_QUEEN[Rows][Columns] = {
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20},
    {-10, 0,   5,   0,  0,  0,  0,  5,   0,   -10},
    {-10, 5,   5,   5,  5,  5,  5,  5,   5,   -10},
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-10, 0,   5,   5,  5,  5,  5,  5,   0,   -10},
    {-10, 0,   0,   0,  0,  0,  0,  0,   0,   -10},
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20}
};

static const int PST_KING_MID[Rows][Columns] = {
    {20,  30,  10,  0,   0,   0,   0,   10,  30,  20 },
    {20,  20,  0,   -5,  -5,  -5,  -5,  0,   20,  20 },
    {-10, -20, -20, -20, -20, -20, -20, -20, -20, -10},
    {-20, -30, -30, -40, -40, -40, -40, -30, -30, -20},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30}
};

static const int PST_KING_END[Rows][Columns] = {
    {-50, -40, -30, -20, -20, -20, -20, -30, -40, -50},
    {-30, -15, -5,  5,   5,   5,   5,   -5,  -15, -30},
    {-20, -5,  10,  15,  20,  20,  15,  10,  -5,  -20},
    {-10, 0,   15,  25,  30,  30,  25,  15,  0,   -10},
    {-10, 0,   15,  25,  30,  30,  25,  15,  0,   -10},
    {-20, -5,  10,  15,  20,  20,  15,  10,  -5,  -20},
    {-30, -15, -5,  5,   5,   5,   5,   -5,  -15, -30},
    {-50, -40, -30, -20, -20, -20, -20, -30, -40, -50}
};

static const int PST_ANTEATER[Rows][Columns] = {
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
int ai_piece_value(PieceType type) {
    switch (type) {
    case PieceType::Ant:
        return 100;
    case PieceType::Knight:
        return 320;
    case PieceType::Bishop:
        return 335;
    case PieceType::Rook:
        return 510;
    case PieceType::Queen:
        return 950;
    case PieceType::King:
        return 20000;
    case PieceType::Anteater:
        return 235;
    case PieceType::Empty:
    default:
        return 0;
    }
}

int ai_phase_value(PieceType type) {
    switch (type) {
    case PieceType::Knight:
        return 1;
    case PieceType::Bishop:
        return 1;
    case PieceType::Rook:
        return 2;
    case PieceType::Queen:
        return 4;
    case PieceType::Anteater:
        return 1;
    case PieceType::Ant:
    case PieceType::King:
    case PieceType::Empty:
    default:
        return 0;
    }
}

int ai_mobility_weight(PieceType type) {
    switch (type) {
    case PieceType::Ant:
        return 4;
    case PieceType::Knight:
        return 5;
    case PieceType::Bishop:
        return 5;
    case PieceType::Rook:
        return 3;
    case PieceType::Queen:
        return 2;
    case PieceType::Anteater:
        return 6;
    case PieceType::King:
    case PieceType::Empty:
    default:
        return 0;
    }
}

static int ai_board_row_for_pst(Color color, int row) {
    return (color == Color::White) ? (Rows - 1 - row) : row;
}

static int ai_king_pst_bonus(Piece piece, int row, int col, int phase) {
    int pstRow;
    int midScore;
    int endScore;

    pstRow = ai_board_row_for_pst(piece.color, row);
    midScore = PST_KING_MID[pstRow][col];
    endScore = PST_KING_END[pstRow][col];
    // Phase AI_MAX_PHASE selects the opening table; phase zero selects the endgame.
    return (midScore * phase + endScore * (AI_MAX_PHASE - phase)) / AI_MAX_PHASE;
}

int ai_pst_bonus(Piece piece, int row, int col, int phase) {
    int pstRow;

    pstRow = ai_board_row_for_pst(piece.color, row);
    switch (piece.type) {
    case PieceType::Ant:
        return PST_ANT[pstRow][col];
    case PieceType::Knight:
        return PST_KNIGHT[pstRow][col];
    case PieceType::Bishop:
        return PST_BISHOP[pstRow][col];
    case PieceType::Rook:
        return PST_ROOK[pstRow][col];
    case PieceType::Queen:
        return PST_QUEEN[pstRow][col];
    case PieceType::King:
        return ai_king_pst_bonus(piece, row, col, phase);
    case PieceType::Anteater:
        return PST_ANTEATER[pstRow][col];
    case PieceType::Empty:
    default:
        return 0;
    }
}

} // namespace ac
