#include "anteater/rules.hpp"
#include "score_constants.hpp"
#include "piece_tables.hpp"
#include "move_facts.hpp"
#include "see.hpp"

namespace ac {

/* Heuristic exchange/attack estimates for search; Rules remains the authority
 * for actual move legality. Preserve the original SEE approximations. */

static int ai_is_path_clear_for_attack(const Board *board, Square from, Square target) {
    int rowStep;
    int colStep;
    Square current;

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

    while (!position_equal(current, target)) {
        if (get_piece(board, current).type != PieceType::Empty) {
            return 0;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 1;
}

static int ai_ant_attacks_square(Square from, Piece piece, Square target) {
    int direction;

    direction = (piece.color == Color::White) ? -1 : 1;

    // target is in correct row direction and in diagonal position
    return (target.row - from.row) == direction && ai_absolute_value(target.col - from.col) == 1;
}

static int ai_rook_attacks_square(const Board *board, Square from, Square target) {
    // target not in same column or row
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    // call helper function to check if path is clear (without including diagonal)
    return ai_is_path_clear_for_attack(board, from, target);
}

static int ai_bishop_attacks_square(const Board *board, Square from, Square target) {
    if (ai_absolute_value(target.row - from.row) != ai_absolute_value(target.col - from.col)) {
        return 0;
    }
    return ai_is_path_clear_for_attack(board, from, target);
}

static int ai_knight_attacks_square(Square from, Square target) {
    int rowDistance;
    int colDistance;

    rowDistance = ai_absolute_value(target.row - from.row);
    colDistance = ai_absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
}

static int ai_king_attacks_square(Square from, Square target) {
    int rowDistance;
    int colDistance;

    rowDistance = ai_absolute_value(target.row - from.row);
    colDistance = ai_absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !position_equal(from, target);
}

static int ai_anteater_attacks_piece_for_see(const Board *board, Square from, Piece piece, Square target,
                                             Piece targetPiece) {
    int rowDistance;
    int colDistance;
    int rowStep;
    int colStep;
    Square current;

    if (piece.type != PieceType::Anteater || targetPiece.type != PieceType::Ant) {
        return 0;
    }

    rowDistance = ai_absolute_value(target.row - from.row);
    colDistance = ai_absolute_value(target.col - from.col);
    if (rowDistance <= 1 && colDistance <= 1 && !position_equal(from, target)) {
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

    current = create_position(from.row + rowStep, from.col + colStep);
    while (is_valid_position(current)) {
        Piece occupant = get_piece(board, current);

        if (occupant.type != PieceType::Ant || occupant.color == piece.color) {
            return 0;
        }
        if (position_equal(current, target)) {
            return 1;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 0;
}

static int ai_piece_attacks_square_for_see(const Board *board, Square from, Piece piece, Square target,
                                           Piece targetPiece) {
    switch (piece.type) {
    case PieceType::Ant:
        return ai_ant_attacks_square(from, piece, target);
    case PieceType::Rook:
        return ai_rook_attacks_square(board, from, target);
    case PieceType::Knight:
        return ai_knight_attacks_square(from, target);
    case PieceType::Bishop:
        return ai_bishop_attacks_square(board, from, target);
    case PieceType::Queen:
        return ai_rook_attacks_square(board, from, target) || ai_bishop_attacks_square(board, from, target);
    case PieceType::King:
        return ai_king_attacks_square(from, target);
    case PieceType::Anteater:
        return ai_anteater_attacks_piece_for_see(board, from, piece, target, targetPiece);
    case PieceType::Empty:
    default:
        return 0;
    }
}

static int ai_square_is_attacked_for_king(const Board *board, Square target, Color attackingColor) {
    int row;
    int col;

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Square from = create_position(row, col);
            Piece piece = get_piece(board, from);
            Piece kingTarget =
                create_piece(PieceType::King, (attackingColor == Color::White) ? Color::Black : Color::White);

            // skip empty and friendly
            if (piece.type == PieceType::Empty || piece.color != attackingColor) {
                continue;
            }
            // anteater cannot eat king
            if (piece.type == PieceType::Anteater) {
                continue;
            }
            if (ai_piece_attacks_square_for_see(board, from, piece, target, kingTarget) == 1) {
                return 1;
            }
        }
    }

    return 0;
}

static Piece ai_piece_after_see_capture(Piece piece, Square target) {
    if (piece.type == PieceType::Ant &&
        ((piece.color == Color::White && target.row == 0) || (piece.color == Color::Black && target.row == Rows - 1))) {
        return create_piece(PieceType::Queen, piece.color);
    }

    return piece;
}

static void ai_apply_move_to_board_for_see(Board *board, Move move) {
    Piece placedPiece;
    int captureIndex;

    // remove the attacker and all capture pieces from the board
    remove_piece(board, move.from);
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        remove_piece(board, move.captures[captureIndex].pos);
    }

    placedPiece = move.movedPiece;

    // in case of promotion, change the piece type
    if (move.specialType == SpecialMove::PromotionQueen) {
        placedPiece = create_piece(PieceType::Queen, move.movedPiece.color);
    } else if (move.specialType == SpecialMove::PromotionRook) {
        placedPiece = create_piece(PieceType::Rook, move.movedPiece.color);
    } else if (move.specialType == SpecialMove::PromotionBishop) {
        placedPiece = create_piece(PieceType::Bishop, move.movedPiece.color);
    } else if (move.specialType == SpecialMove::PromotionKnight) {
        placedPiece = create_piece(PieceType::Knight, move.movedPiece.color);
    }

    set_piece(board, move.to, placedPiece);
}

static int ai_king_capture_is_legal_for_see(const Board *board, Square from, Piece piece, Square target) {
    Board trial;
    Color enemyColor;

    trial = *board;
    remove_piece(&trial, from);
    set_piece(&trial, target, piece);
    enemyColor = (piece.color == Color::White) ? Color::Black : Color::White;
    return ai_square_is_attacked_for_king(&trial, target, enemyColor) == 0;
}

static int ai_find_least_valuable_attacker(const Board *board, Square target, Color side, Square *fromOut,
                                           Piece *pieceOut, int *valueOut) {
    int row;
    int col;
    int bestValue;
    int found;
    Piece targetPiece;

    if (board == NULL || fromOut == NULL || pieceOut == NULL || valueOut == NULL) {
        return 0;
    }

    targetPiece = get_piece(board, target);
    bestValue = AI_INF; // set to infinity to get the lowest value
    found = 0;
    // iterate all board to find the least valuable attacker
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Square from = create_position(row, col);
            Piece piece = get_piece(board, from);
            int value;

            // skip EMPTY
            if (piece.type == PieceType::Empty || piece.color != side) {
                continue;
            }
            // skip PieceType::King if it will be captured in case of capture it
            if (piece.type == PieceType::King && ai_king_capture_is_legal_for_see(board, from, piece, target) == 0) {
                continue;
            }
            // skip if the piece does not attack the target
            if (ai_piece_attacks_square_for_see(board, from, piece, target, targetPiece) == 0) {
                continue;
            }

            value = ai_piece_value(piece.type);
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

int ai_see_initial_gain(const Move *move) {
    int gain;
    int captureIndex;

    gain = 0;
    // sum all the value of the captured pieces
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        gain += ai_piece_value(move->captures[captureIndex].piece.type);
    }
    // if promotion, add the value of the new piece
    if (ai_is_promotion_move(move)) {
        gain += ai_piece_value(PieceType::Queen) - ai_piece_value(PieceType::Ant);
    }

    return gain;
}

int ai_see_move_score(const Position *state, const Move *move) {
    Board board;
    Square target;
    Color side;
    int gain[32];
    int depth;
    Piece occupant;

    if (state == NULL || move == NULL || !ai_is_noisy_move(move)) {
        return 0;
    }

    board = state->board;                                                          // get a copy of the board
    target = move->to;                                                             // get the target position
    gain[0] = ai_see_initial_gain(move);                                           // get the initial gain
    ai_apply_move_to_board_for_see(&board, *move);                                 // apply the move to the copy
    occupant = get_piece(&board, target);                                          // get the piece at the target
    side = (move->movedPiece.color == Color::White) ? Color::Black : Color::White; // get the next attack
    depth = 1;

    // set limitation as 32
    while (depth < (int)(sizeof(gain) / sizeof(gain[0]))) {
        // Successful lookup overwrites these outputs. Initialization also avoids
        // GCC 13 false positives after the private helper is inlined.
        Square from = {};
        Piece attacker = {};
        int attackerValue = 0;

        // find the least valuable attacker
        if (ai_find_least_valuable_attacker(&board, target, side, &from, &attacker, &attackerValue) == 0) {
            break;
        }

        gain[depth] = ai_piece_value(occupant.type) - gain[depth - 1];

        // early pruning if both side losing material
        if ((gain[depth] < 0) && (-gain[depth - 1] < 0)) {
            break;
        }

        // apply the move to the copy
        remove_piece(&board, from);
        occupant = ai_piece_after_see_capture(attacker, target);
        set_piece(&board, target, occupant);
        side = (side == Color::White) ? Color::Black : Color::White;
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

} // namespace ac
