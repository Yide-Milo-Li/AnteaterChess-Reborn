#include "internal.h"
#include <stdlib.h>
int ac_is_in_check(const AcPosition *s, AcColor c) {
    if (!s || (c != AC_WHITE && c != AC_BLACK))
        return 0;
    for (int r = 0; r < AC_ROWS; ++r)
        for (int k = 0; k < AC_COLS; ++k) {
            AcPiece p = s->board.cells[r][k];
            if (p.type == AC_KING && p.color == c)
                return ac_square_attacked(&s->board, ac_create_position(r, k), c == AC_WHITE ? AC_BLACK : AC_WHITE);
        }
    return 0;
}
int ac_is_insufficient_material(const AcPosition *state) {
    int totalNonKingPieces;
    int totalAnts;
    int totalRooks;
    int totalQueens;
    int totalBishops;
    int totalKnights;
    int totalAnteaters;
    int bishopsAllSameColor;
    int firstBishopSquareColor;
    int row;
    int col;

    if (state == NULL) {
        return 0;
    }

    totalNonKingPieces = 0;
    totalAnts = 0;
    totalRooks = 0;
    totalQueens = 0;
    totalBishops = 0;
    totalKnights = 0;
    totalAnteaters = 0;
    bishopsAllSameColor = 1;
    firstBishopSquareColor = -1;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));
            int squareColor;

            /* Kings do not count toward mating material, and empty squares are
             * ignored entirely. */
            if (piece.type == AC_EMPTY_PIECE || piece.type == AC_KING) {
                continue;
            }

            ++totalNonKingPieces;

            /* Count each remaining piece type so the draw rules can make a
             * simple decision after the full board scan is done. */
            if (piece.type == AC_ANT) {
                ++totalAnts;
            } else if (piece.type == AC_ROOK) {
                ++totalRooks;
            } else if (piece.type == AC_QUEEN) {
                ++totalQueens;
            } else if (piece.type == AC_BISHOP) {
                ++totalBishops;

                /* Same-color bishops are a common insufficient-material case,
                 * so remember the color of the first bishop square and compare
                 * every later bishop against it. */
                squareColor = (row + col) % 2;

                if (firstBishopSquareColor == -1) {
                    firstBishopSquareColor = squareColor;
                } else if (firstBishopSquareColor != squareColor) {
                    bishopsAllSameColor = 0;
                }
            } else if (piece.type == AC_KNIGHT) {
                ++totalKnights;
            } else if (piece.type == AC_ANTEATER) {
                ++totalAnteaters;
            }
        }
    }

    /* Any remaining ant, rook, or queen is treated as enough material to keep
     * the game alive. */
    if (totalAnts > 0 || totalRooks > 0 || totalQueens > 0) {
        return 0;
    }

    /* King versus king is always a draw. */
    if (totalNonKingPieces == 0) {
        return 1;
    }

    /* Only anteaters remain besides the kings. Under this ruleset they do not
     * supply mating material on their own. */
    if (totalBishops == 0 && totalKnights == 0) {
        return 1;
    }

    /* A single bishop cannot force mate with only the two kings on board. */
    if (totalBishops == 1 && totalKnights == 0) {
        return 1;
    }

    /* A single knight also counts as insufficient material here. */
    if (totalKnights == 1 && totalBishops == 0) {
        return 1;
    }

    /* Two knights are still treated as insufficient when no other supporting
     * piece types remain. */
    if (totalKnights == 2 && totalBishops == 0 && totalAnteaters == 0) {
        return 1;
    }

    /* Multiple bishops that all live on the same color squares are also
     * treated as insufficient in this simplified detector. */
    if (totalBishops > 0 && totalKnights == 0 && bishopsAllSameColor == 1) {
        return 1;
    }

    /* Any other combination is treated as enough material to continue play. */
    return 0;
}

AcStatus ac_position_result(const AcPosition *s, AcGameResult *result) {
    if (!s || !result)
        return AC_INVALID_ARGUMENT;
    AcMoveList *moves = malloc(sizeof(*moves));
    if (!moves)
        return AC_OUT_OF_MEMORY;
    AcStatus status = ac_generate_legal_moves(s, moves);
    int count = moves->count;
    free(moves);
    if (status != AC_OK)
        return status;
    *result = AC_RESULT_NONE;
    if (count == 0)
        *result = ac_is_in_check(s, s->currentTurn)
                      ? (s->currentTurn == AC_WHITE ? AC_RESULT_BLACK_WIN : AC_RESULT_WHITE_WIN)
                      : AC_RESULT_DRAW;
    else if (ac_is_insufficient_material(s))
        *result = AC_RESULT_DRAW;
    return AC_OK;
}
