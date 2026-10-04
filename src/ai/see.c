#include "anteater/rules.h"
#include "score_constants.h"
#include "piece_tables.h"
#include "move_facts.h"
#include "see.h"

/* Heuristic exchange/attack estimates for search; Rules remains the authority
 * for actual move legality. Preserve the original SEE approximations. */

static int ac_ai_is_path_clear_for_attack(const AcBoard *board, AcSquare from, AcSquare target) {
    int rowStep;
    int colStep;
    AcSquare current;

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

    while (!ac_position_equal(current, target)) {
        if (ac_get_piece(board, current).type != AC_EMPTY_PIECE) {
            return 0;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 1;
}

static int ac_ai_ant_attacks_square(AcSquare from, AcPiece piece, AcSquare target) {
    int direction;

    direction = (piece.color == AC_WHITE) ? -1 : 1;

    // target is in correct row direction and in diagonal position
    return (target.row - from.row) == direction && ac_ai_absolute_value(target.col - from.col) == 1;
}

static int ac_ai_rook_attacks_square(const AcBoard *board, AcSquare from, AcSquare target) {
    // target not in same column or row
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    // call helper function to check if path is clear (without including diagonal)
    return ac_ai_is_path_clear_for_attack(board, from, target);
}

static int ac_ai_bishop_attacks_square(const AcBoard *board, AcSquare from, AcSquare target) {
    if (ac_ai_absolute_value(target.row - from.row) != ac_ai_absolute_value(target.col - from.col)) {
        return 0;
    }
    return ac_ai_is_path_clear_for_attack(board, from, target);
}

static int ac_ai_knight_attacks_square(AcSquare from, AcSquare target) {
    int rowDistance;
    int colDistance;

    rowDistance = ac_ai_absolute_value(target.row - from.row);
    colDistance = ac_ai_absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
}

static int ac_ai_king_attacks_square(AcSquare from, AcSquare target) {
    int rowDistance;
    int colDistance;

    rowDistance = ac_ai_absolute_value(target.row - from.row);
    colDistance = ac_ai_absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !ac_position_equal(from, target);
}

static int ac_ai_anteater_attacks_piece_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target,
                                         AcPiece targetPiece) {
    int rowDistance;
    int colDistance;
    int rowStep;
    int colStep;
    AcSquare current;

    if (piece.type != AC_ANTEATER || targetPiece.type != AC_ANT) {
        return 0;
    }

    rowDistance = ac_ai_absolute_value(target.row - from.row);
    colDistance = ac_ai_absolute_value(target.col - from.col);
    if (rowDistance <= 1 && colDistance <= 1 && !ac_position_equal(from, target)) {
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

    current = ac_create_position(from.row + rowStep, from.col + colStep);
    while (ac_is_valid_position(current)) {
        AcPiece occupant = ac_get_piece(board, current);

        if (occupant.type != AC_ANT || occupant.color == piece.color) {
            return 0;
        }
        if (ac_position_equal(current, target)) {
            return 1;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 0;
}

static int ac_ai_piece_attacks_square_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target,
                                       AcPiece targetPiece) {
    switch (piece.type) {
    case AC_ANT:
        return ac_ai_ant_attacks_square(from, piece, target);
    case AC_ROOK:
        return ac_ai_rook_attacks_square(board, from, target);
    case AC_KNIGHT:
        return ac_ai_knight_attacks_square(from, target);
    case AC_BISHOP:
        return ac_ai_bishop_attacks_square(board, from, target);
    case AC_QUEEN:
        return ac_ai_rook_attacks_square(board, from, target) || ac_ai_bishop_attacks_square(board, from, target);
    case AC_KING:
        return ac_ai_king_attacks_square(from, target);
    case AC_ANTEATER:
        return ac_ai_anteater_attacks_piece_for_see(board, from, piece, target, targetPiece);
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

static int ac_ai_square_is_attacked_for_king(const AcBoard *board, AcSquare target, AcColor attackingColor) {
    int row;
    int col;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare from = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(board, from);
            AcPiece kingTarget = ac_create_piece(AC_KING, (attackingColor == AC_WHITE) ? AC_BLACK : AC_WHITE);

            // skip empty and friendly
            if (piece.type == AC_EMPTY_PIECE || piece.color != attackingColor) {
                continue;
            }
            // anteater cannot eat king
            if (piece.type == AC_ANTEATER) {
                continue;
            }
            if (ac_ai_piece_attacks_square_for_see(board, from, piece, target, kingTarget) == 1) {
                return 1;
            }
        }
    }

    return 0;
}

static AcPiece ac_ai_piece_after_see_capture(AcPiece piece, AcSquare target) {
    if (piece.type == AC_ANT &&
        ((piece.color == AC_WHITE && target.row == 0) || (piece.color == AC_BLACK && target.row == AC_ROWS - 1))) {
        return ac_create_piece(AC_QUEEN, piece.color);
    }

    return piece;
}

static void ac_ai_apply_move_to_board_for_see(AcBoard *board, AcMove move) {
    AcPiece placedPiece;
    int captureIndex;

    // remove the attacker and all capture pieces from the board
    ac_remove_piece(board, move.from);
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        ac_remove_piece(board, move.captures[captureIndex].pos);
    }

    placedPiece = move.movedPiece;

    // in case of promotion, change the piece type
    if (move.specialType == AC_PROMOTION_QUEEN) {
        placedPiece = ac_create_piece(AC_QUEEN, move.movedPiece.color);
    } else if (move.specialType == AC_PROMOTION_ROOK) {
        placedPiece = ac_create_piece(AC_ROOK, move.movedPiece.color);
    } else if (move.specialType == AC_PROMOTION_BISHOP) {
        placedPiece = ac_create_piece(AC_BISHOP, move.movedPiece.color);
    } else if (move.specialType == AC_PROMOTION_KNIGHT) {
        placedPiece = ac_create_piece(AC_KNIGHT, move.movedPiece.color);
    }

    ac_set_piece(board, move.to, placedPiece);
}

static int ac_ai_king_capture_is_legal_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target) {
    AcBoard trial;
    AcColor enemyColor;

    trial = *board;
    ac_remove_piece(&trial, from);
    ac_set_piece(&trial, target, piece);
    enemyColor = (piece.color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    return ac_ai_square_is_attacked_for_king(&trial, target, enemyColor) == 0;
}

static int ac_ai_find_least_valuable_attacker(const AcBoard *board, AcSquare target, AcColor side, AcSquare *fromOut,
                                       AcPiece *pieceOut, int *valueOut) {
    int row;
    int col;
    int bestValue;
    int found;
    AcPiece targetPiece;

    if (board == NULL || fromOut == NULL || pieceOut == NULL || valueOut == NULL) {
        return 0;
    }

    targetPiece = ac_get_piece(board, target);
    bestValue = AI_INF; // set to infinity to get the lowest value
    found = 0;
    // iterate all board to find the least valuable attacker
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare from = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(board, from);
            int value;

            // skip EMPTY
            if (piece.type == AC_EMPTY_PIECE || piece.color != side) {
                continue;
            }
            // skip AC_KING if it will be captured in case of capture it
            if (piece.type == AC_KING && ac_ai_king_capture_is_legal_for_see(board, from, piece, target) == 0) {
                continue;
            }
            // skip if the piece does not attack the target
            if (ac_ai_piece_attacks_square_for_see(board, from, piece, target, targetPiece) == 0) {
                continue;
            }

            value = ac_ai_piece_value(piece.type);
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

int ac_ai_see_initial_gain(const AcMove *move) {
    int gain;
    int captureIndex;

    gain = 0;
    // sum all the value of the captured pieces
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        gain += ac_ai_piece_value(move->captures[captureIndex].piece.type);
    }
    // if promotion, add the value of the new piece
    if (ac_ai_is_promotion_move(move)) {
        gain += ac_ai_piece_value(AC_QUEEN) - ac_ai_piece_value(AC_ANT);
    }

    return gain;
}

int ac_ai_see_move_score(const AcPosition *state, const AcMove *move) {
    AcBoard board;
    AcSquare target;
    AcColor side;
    int gain[32];
    int depth;
    AcPiece occupant;

    if (state == NULL || move == NULL || !ac_ai_is_noisy_move(move)) {
        return 0;
    }

    board = state->board;                                              // get a copy of the board
    target = move->to;                                                 // get the target position
    gain[0] = ac_ai_see_initial_gain(move);                            // get the initial gain
    ac_ai_apply_move_to_board_for_see(&board, *move);                  // apply the move to the copy
    occupant = ac_get_piece(&board, target);                           // get the piece at the target
    side = (move->movedPiece.color == AC_WHITE) ? AC_BLACK : AC_WHITE; // get the next attack
    depth = 1;

    // set limitation as 32
    while (depth < (int)(sizeof(gain) / sizeof(gain[0]))) {
        // Successful lookup overwrites these outputs. Initialization also avoids
        // GCC 13 false positives after the private helper is inlined.
        AcSquare from = {0};
        AcPiece attacker = {0};
        int attackerValue = 0;

        // find the least valuable attacker
        if (ac_ai_find_least_valuable_attacker(&board, target, side, &from, &attacker, &attackerValue) == 0) {
            break;
        }

        gain[depth] = ac_ai_piece_value(occupant.type) - gain[depth - 1];

        // early pruning if both side losing material
        if ((gain[depth] < 0) && (-gain[depth - 1] < 0)) {
            break;
        }

        // apply the move to the copy
        ac_remove_piece(&board, from);
        occupant = ac_ai_piece_after_see_capture(attacker, target);
        ac_set_piece(&board, target, occupant);
        side = (side == AC_WHITE) ? AC_BLACK : AC_WHITE;
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

