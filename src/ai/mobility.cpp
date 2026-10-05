#include "anteater/rules.h"
#include "score_constants.h"
#include "mobility.h"

/* Mobility estimates count candidate geometry, not complete legal moves. */

static int ac_ai_count_sliding_mobility(const AcBoard *board, AcSquare from, AcPiece piece, int rowStep, int colStep) {
    AcSquare current;
    int mobility;

    current = from;
    current.row += rowStep;
    current.col += colStep;
    mobility = 0;
    while (ac_is_valid_position(current)) {
        AcPiece target = ac_get_piece(board, current);

        if (target.type == AC_EMPTY_PIECE) {
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

static int ac_ai_count_ant_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
    int direction;
    int mobility;
    AcSquare forward;
    int fileOffset;

    direction = (piece.color == AC_WHITE) ? -1 : 1;
    mobility = 0;

    // one step forward
    forward = ac_create_position(from.row + direction, from.col);
    if (ac_is_valid_position(forward) && ac_get_piece(board, forward).type == AC_EMPTY_PIECE) {
        ++mobility;

        // two step forward
        if (((piece.color == AC_WHITE && from.row == 6) || (piece.color == AC_BLACK && from.row == 1))) {
            AcSquare doubleStep = ac_create_position(from.row + (2 * direction), from.col);

            if (ac_is_valid_position(doubleStep) && ac_get_piece(board, doubleStep).type == AC_EMPTY_PIECE) {
                ++mobility;
            }
        }
    }

    // diagonal
    for (fileOffset = -1; fileOffset <= 1; fileOffset += 2) {
        AcSquare diagonal = ac_create_position(from.row + direction, from.col + fileOffset);
        AcPiece target;

        if (!ac_is_valid_position(diagonal)) {
            continue;
        }

        target = ac_get_piece(board, diagonal);
        if (target.type != AC_EMPTY_PIECE && target.color != piece.color) {
            ++mobility;
        }
    }

    return mobility;
}

static int ac_ai_count_knight_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
    // knight moves
    static const int rowOffsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    static const int colOffsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};
    int index;
    int mobility;

    mobility = 0;
    for (index = 0; index < 8; ++index) {
        AcSquare target = ac_create_position(from.row + rowOffsets[index], from.col + colOffsets[index]);
        AcPiece occupant;

        if (!ac_is_valid_position(target)) {
            continue;
        }

        occupant = ac_get_piece(board, target);
        if (occupant.type == AC_EMPTY_PIECE || occupant.color != piece.color) {
            ++mobility;
        }
    }

    return mobility;
}

static int ac_ai_count_anteater_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
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
        AcSquare target = ac_create_position(from.row + rowSteps[index], from.col + colSteps[index]);
        AcPiece occupant;

        if (!ac_is_valid_position(target)) {
            continue;
        }

        occupant = ac_get_piece(board, target);
        if (occupant.type == AC_EMPTY_PIECE || (occupant.type == AC_ANT && occupant.color != piece.color)) {
            ++mobility;
        }
    }

    for (index = 0; index < 4; ++index) {
        AcSquare current = ac_create_position(from.row + chainRowSteps[index], from.col + chainColSteps[index]);
        int chainLength;

        chainLength = 0;
        while (ac_is_valid_position(current)) {
            AcPiece target = ac_get_piece(board, current);

            if (target.type != AC_ANT || target.color == piece.color) {
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

int ac_ai_count_king_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
    int rowOffset;
    int colOffset;
    int mobility;

    mobility = 0;
    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            AcSquare target;
            AcPiece occupant;

            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            target = ac_create_position(from.row + rowOffset, from.col + colOffset);
            if (!ac_is_valid_position(target)) {
                continue;
            }

            occupant = ac_get_piece(board, target);
            if (occupant.type == AC_EMPTY_PIECE || occupant.color != piece.color) {
                ++mobility;
            }
        }
    }

    return mobility;
}

int ac_ai_piece_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
    switch (piece.type) {
    case AC_ANT:
        return ac_ai_count_ant_mobility(board, from, piece);
    case AC_ROOK:
        // 4 sliding directions
        return ac_ai_count_sliding_mobility(board, from, piece, -1, 0) +
               ac_ai_count_sliding_mobility(board, from, piece, 1, 0) +
               ac_ai_count_sliding_mobility(board, from, piece, 0, -1) +
               ac_ai_count_sliding_mobility(board, from, piece, 0, 1);
    case AC_BISHOP:
        // 4 sliding directions
        return ac_ai_count_sliding_mobility(board, from, piece, -1, -1) +
               ac_ai_count_sliding_mobility(board, from, piece, -1, 1) +
               ac_ai_count_sliding_mobility(board, from, piece, 1, -1) +
               ac_ai_count_sliding_mobility(board, from, piece, 1, 1);
    case AC_QUEEN:
        // 8 sliding directions
        return ac_ai_count_sliding_mobility(board, from, piece, -1, 0) +
               ac_ai_count_sliding_mobility(board, from, piece, 1, 0) +
               ac_ai_count_sliding_mobility(board, from, piece, 0, -1) +
               ac_ai_count_sliding_mobility(board, from, piece, 0, 1) +
               ac_ai_count_sliding_mobility(board, from, piece, -1, -1) +
               ac_ai_count_sliding_mobility(board, from, piece, -1, 1) +
               ac_ai_count_sliding_mobility(board, from, piece, 1, -1) +
               ac_ai_count_sliding_mobility(board, from, piece, 1, 1);
    case AC_KNIGHT:
        return ac_ai_count_knight_mobility(board, from, piece);
    case AC_KING:
        return ac_ai_count_king_mobility(board, from, piece);
    case AC_ANTEATER:
        return ac_ai_count_anteater_mobility(board, from, piece);
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

