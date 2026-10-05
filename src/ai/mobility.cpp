#include "anteater/rules.hpp"
#include "score_constants.hpp"
#include "mobility.hpp"

namespace ac {

/* Mobility estimates count candidate geometry, not complete legal moves. */

static int ai_count_sliding_mobility(const Board *board, Square from, Piece piece, int rowStep, int colStep) {
    Square current;
    int mobility;

    current = from;
    current.row += rowStep;
    current.col += colStep;
    mobility = 0;
    while (is_valid_position(current)) {
        Piece target = get_piece(board, current);

        if (target.type == PieceType::Empty) {
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

static int ai_count_ant_mobility(const Board *board, Square from, Piece piece) {
    int direction;
    int mobility;
    Square forward;
    int fileOffset;

    direction = (piece.color == Color::White) ? -1 : 1;
    mobility = 0;

    // one step forward
    forward = create_position(from.row + direction, from.col);
    if (is_valid_position(forward) && get_piece(board, forward).type == PieceType::Empty) {
        ++mobility;

        // two step forward
        if (((piece.color == Color::White && from.row == 6) || (piece.color == Color::Black && from.row == 1))) {
            Square doubleStep = create_position(from.row + (2 * direction), from.col);

            if (is_valid_position(doubleStep) && get_piece(board, doubleStep).type == PieceType::Empty) {
                ++mobility;
            }
        }
    }

    // diagonal
    for (fileOffset = -1; fileOffset <= 1; fileOffset += 2) {
        Square diagonal = create_position(from.row + direction, from.col + fileOffset);
        Piece target;

        if (!is_valid_position(diagonal)) {
            continue;
        }

        target = get_piece(board, diagonal);
        if (target.type != PieceType::Empty && target.color != piece.color) {
            ++mobility;
        }
    }

    return mobility;
}

static int ai_count_knight_mobility(const Board *board, Square from, Piece piece) {
    // knight moves
    static const int rowOffsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    static const int colOffsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};
    int index;
    int mobility;

    mobility = 0;
    for (index = 0; index < 8; ++index) {
        Square target = create_position(from.row + rowOffsets[index], from.col + colOffsets[index]);
        Piece occupant;

        if (!is_valid_position(target)) {
            continue;
        }

        occupant = get_piece(board, target);
        if (occupant.type == PieceType::Empty || occupant.color != piece.color) {
            ++mobility;
        }
    }

    return mobility;
}

static int ai_count_anteater_mobility(const Board *board, Square from, Piece piece) {
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
        Square target = create_position(from.row + rowSteps[index], from.col + colSteps[index]);
        Piece occupant;

        if (!is_valid_position(target)) {
            continue;
        }

        occupant = get_piece(board, target);
        if (occupant.type == PieceType::Empty || (occupant.type == PieceType::Ant && occupant.color != piece.color)) {
            ++mobility;
        }
    }

    for (index = 0; index < 4; ++index) {
        Square current = create_position(from.row + chainRowSteps[index], from.col + chainColSteps[index]);
        int chainLength;

        chainLength = 0;
        while (is_valid_position(current)) {
            Piece target = get_piece(board, current);

            if (target.type != PieceType::Ant || target.color == piece.color) {
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

int ai_count_king_mobility(const Board *board, Square from, Piece piece) {
    int rowOffset;
    int colOffset;
    int mobility;

    mobility = 0;
    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            Square target;
            Piece occupant;

            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            target = create_position(from.row + rowOffset, from.col + colOffset);
            if (!is_valid_position(target)) {
                continue;
            }

            occupant = get_piece(board, target);
            if (occupant.type == PieceType::Empty || occupant.color != piece.color) {
                ++mobility;
            }
        }
    }

    return mobility;
}

int ai_piece_mobility(const Board *board, Square from, Piece piece) {
    switch (piece.type) {
    case PieceType::Ant:
        return ai_count_ant_mobility(board, from, piece);
    case PieceType::Rook:
        // 4 sliding directions
        return ai_count_sliding_mobility(board, from, piece, -1, 0) +
               ai_count_sliding_mobility(board, from, piece, 1, 0) +
               ai_count_sliding_mobility(board, from, piece, 0, -1) +
               ai_count_sliding_mobility(board, from, piece, 0, 1);
    case PieceType::Bishop:
        // 4 sliding directions
        return ai_count_sliding_mobility(board, from, piece, -1, -1) +
               ai_count_sliding_mobility(board, from, piece, -1, 1) +
               ai_count_sliding_mobility(board, from, piece, 1, -1) +
               ai_count_sliding_mobility(board, from, piece, 1, 1);
    case PieceType::Queen:
        // 8 sliding directions
        return ai_count_sliding_mobility(board, from, piece, -1, 0) +
               ai_count_sliding_mobility(board, from, piece, 1, 0) +
               ai_count_sliding_mobility(board, from, piece, 0, -1) +
               ai_count_sliding_mobility(board, from, piece, 0, 1) +
               ai_count_sliding_mobility(board, from, piece, -1, -1) +
               ai_count_sliding_mobility(board, from, piece, -1, 1) +
               ai_count_sliding_mobility(board, from, piece, 1, -1) +
               ai_count_sliding_mobility(board, from, piece, 1, 1);
    case PieceType::Knight:
        return ai_count_knight_mobility(board, from, piece);
    case PieceType::King:
        return ai_count_king_mobility(board, from, piece);
    case PieceType::Anteater:
        return ai_count_anteater_mobility(board, from, piece);
    case PieceType::Empty:
    default:
        return 0;
    }
}

} // namespace ac
