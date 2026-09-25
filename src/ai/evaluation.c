#include "internal.h"
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

int ac_ai_board_row_for_pst(AcColor color, int row) {
    return (color == AC_WHITE) ? (AC_ROWS - 1 - row) : row;
}

int ac_ai_king_pst_bonus(AcPiece piece, int row, int col, int phase) {
    int pstRow;
    int midScore;
    int endScore;

    pstRow = ac_ai_board_row_for_pst(piece.color, row);
    midScore = PST_KING_MID[pstRow][col];
    endScore = PST_KING_END[pstRow][col];
    // Weighted average with phase = 0 is opening, phase = AI_MAX_PHASE is endgame
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

int ac_ai_is_promotion_move(const AcMove *move) {
    return move != NULL && ac_is_promotion_special_move(move->specialType);
}

int ac_ai_is_noisy_move(const AcMove *move) {
    return move->captureCount > 0 || move->specialType == AC_ANTEATER_CAPTURE || ac_ai_is_promotion_move(move);
}

int ac_ai_is_quiet_move(const AcMove *move) {
    return !ac_ai_is_noisy_move(move) && move->specialType != AC_CASTLING_KINGSIDE &&
           move->specialType != AC_CASTLING_QUEENSIDE;
}

int ac_ai_square_index(AcSquare pos) {
    return pos.row * AC_COLS + pos.col;
}

int ac_ai_absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

int ac_ai_is_irreversible_move(const AcMove *move) {
    if (move == NULL) {
        return 0;
    }

    return move->movedPiece.type == AC_ANT || move->captureCount > 0 || ac_ai_is_promotion_move(move);
}

int ac_ai_is_path_clear_for_attack(const AcBoard *board, AcSquare from, AcSquare target) {
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

int ac_ai_ant_attacks_square(AcSquare from, AcPiece piece, AcSquare target) {
    int direction;

    direction = (piece.color == AC_WHITE) ? -1 : 1;

    // target is in correct row direction and in diagonal position
    return (target.row - from.row) == direction && ac_ai_absolute_value(target.col - from.col) == 1;
}

int ac_ai_rook_attacks_square(const AcBoard *board, AcSquare from, AcSquare target) {
    // target not in same column or row
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    // call helper function to check if path is clear (without including diagonal)
    return ac_ai_is_path_clear_for_attack(board, from, target);
}

int ac_ai_bishop_attacks_square(const AcBoard *board, AcSquare from, AcSquare target) {
    if (ac_ai_absolute_value(target.row - from.row) != ac_ai_absolute_value(target.col - from.col)) {
        return 0;
    }
    return ac_ai_is_path_clear_for_attack(board, from, target);
}

int ac_ai_knight_attacks_square(AcSquare from, AcSquare target) {
    int rowDistance;
    int colDistance;

    rowDistance = ac_ai_absolute_value(target.row - from.row);
    colDistance = ac_ai_absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
}

int ac_ai_king_attacks_square(AcSquare from, AcSquare target) {
    int rowDistance;
    int colDistance;

    rowDistance = ac_ai_absolute_value(target.row - from.row);
    colDistance = ac_ai_absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !ac_position_equal(from, target);
}

int ac_ai_anteater_attacks_piece_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target,
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

int ac_ai_piece_attacks_square_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target,
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

int ac_ai_square_is_attacked_for_king(const AcBoard *board, AcSquare target, AcColor attackingColor) {
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

AcPiece ac_ai_piece_after_see_capture(AcPiece piece, AcSquare target) {
    if (piece.type == AC_ANT &&
        ((piece.color == AC_WHITE && target.row == 0) || (piece.color == AC_BLACK && target.row == AC_ROWS - 1))) {
        return ac_create_piece(AC_QUEEN, piece.color);
    }

    return piece;
}

void ac_ai_apply_move_to_board_for_see(AcBoard *board, AcMove move) {
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

int ac_ai_king_capture_is_legal_for_see(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target) {
    AcBoard trial;
    AcColor enemyColor;

    trial = *board;
    ac_remove_piece(&trial, from);
    ac_set_piece(&trial, target, piece);
    enemyColor = (piece.color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    return ac_ai_square_is_attacked_for_king(&trial, target, enemyColor) == 0;
}

int ac_ai_find_least_valuable_attacker(const AcBoard *board, AcSquare target, AcColor side, AcSquare *fromOut,
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
        AcSquare from;
        AcPiece attacker;
        int attackerValue;

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

int ac_ai_count_sliding_mobility(const AcBoard *board, AcSquare from, AcPiece piece, int rowStep, int colStep) {
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

int ac_ai_count_ant_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
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

int ac_ai_count_knight_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
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

int ac_ai_count_anteater_mobility(const AcBoard *board, AcSquare from, AcPiece piece) {
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

int ac_ai_ant_supports_square(const AcBoard *board, AcColor color, AcSquare target) {
    int supportRow;
    int offset;

    supportRow = target.row + ((color == AC_WHITE) ? 1 : -1);
    for (offset = -1; offset <= 1; offset += 2) {
        AcSquare support = ac_create_position(supportRow, target.col + offset);
        AcPiece piece;

        if (!ac_is_valid_position(support)) {
            continue;
        }

        piece = ac_get_piece(board, support);
        if (piece.type == AC_ANT && piece.color == color) {
            return 1;
        }
    }

    return 0;
}

int ac_ai_enemy_ant_can_attack_square(const AcBoard *board, AcColor color, AcSquare target) {
    AcColor enemyColor;
    int enemyRow;
    int offset;

    enemyColor = (color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    enemyRow = target.row + ((color == AC_WHITE) ? -1 : 1);
    for (offset = -1; offset <= 1; offset += 2) {
        AcSquare attacker = ac_create_position(enemyRow, target.col + offset);
        AcPiece piece;

        if (!ac_is_valid_position(attacker)) {
            continue;
        }

        piece = ac_get_piece(board, attacker);
        if (piece.type == AC_ANT && piece.color == enemyColor) {
            return 1;
        }
    }

    return 0;
}

int ac_ai_evaluate_pawns(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS], int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));
            int adjacentFriends;
            int isPassed;
            int step;
            int advance;
            int passedBonus;

            // Get all alley ants
            if (piece.type != AC_ANT || piece.color != color) {
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
            if (col + 1 < AC_COLS && antFiles[color][col + 1] > 0) {
                adjacentFriends = 1;
            }
            if (!adjacentFriends) {
                score -= 10;
            } else {
                score += 6;
            }

            // check passed pawn
            isPassed = 1;
            if (color == AC_WHITE) {
                for (step = row - 1; step >= 0 && isPassed; --step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        AcPiece enemy;

                        if (file < 0 || file >= AC_COLS) {
                            continue;
                        }

                        enemy = ac_get_piece(&state->board, ac_create_position(step, file));
                        if (enemy.type == AC_ANT && enemy.color == AC_BLACK) {
                            isPassed = 0;
                            break;
                        }
                    }
                }
                advance = AC_ROWS - 1 - row;
            } else {
                for (step = row + 1; step < AC_ROWS && isPassed; ++step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        AcPiece enemy;

                        if (file < 0 || file >= AC_COLS) {
                            continue;
                        }

                        enemy = ac_get_piece(&state->board, ac_create_position(step, file));
                        if (enemy.type == AC_ANT && enemy.color == AC_WHITE) {
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

int ac_ai_evaluate_rook_and_queen_files(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS],
                                        int phase) {
    int score;
    int row;
    int col;
    AcColor enemyColor;

    (void)phase;

    score = 0;
    enemyColor = (color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    // iterate over all rooks and queens
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));

            if (piece.color != color || (piece.type != AC_ROOK && piece.type != AC_QUEEN)) {
                continue;
            }

            // If file without any ants , add bonus
            if (antFiles[color][col] == 0 && antFiles[enemyColor][col] == 0) {
                score += (piece.type == AC_ROOK) ? 20 : 8;
                // If file without any alley ants , add bonus
            } else if (antFiles[color][col] == 0) {
                score += (piece.type == AC_ROOK) ? 12 : 4;
            }

            // Bonus for rook on 7th rank
            if (piece.type == AC_ROOK) {
                if ((color == AC_WHITE && row == 1) || (color == AC_BLACK && row == 6)) {
                    score += 14;
                }
            }
        }
    }

    return score;
}

int ac_ai_evaluate_king_safety(const AcPosition *state, AcColor color, AcSquare kingPos, int phase) {
    int score;
    int homeRow;
    int shieldRow;
    int colOffset;

    if (!ac_is_valid_position(kingPos)) {
        return 0;
    }

    score = 0;
    homeRow = (color == AC_WHITE) ? 7 : 0;
    // Bonus for Castling
    if (kingPos.row == homeRow && (kingPos.col == 3 || kingPos.col == 7)) {
        score += 32;
        // game ending penalty for king on default position
    } else if (phase >= 18 && kingPos.row == homeRow && kingPos.col == 5) {
        score -= 24;
    }

    // Pawn Shield
    shieldRow = kingPos.row + ((color == AC_WHITE) ? -1 : 1);
    for (colOffset = -1; colOffset <= 1; ++colOffset) {
        AcSquare shield = ac_create_position(shieldRow, kingPos.col + colOffset);
        AcPiece occupant;

        if (!ac_is_valid_position(shield)) {
            continue;
        }

        occupant = ac_get_piece(&state->board, shield);
        if (occupant.type == AC_ANT && occupant.color == color) {
            score += 12;
        } else {
            score -= 8;
        }
    }

    // mobility bonus in end game
    if (phase <= 10) {
        score += ac_ai_count_king_mobility(&state->board, kingPos, ac_create_piece(AC_KING, color)) * 2;
    }

    return score;
}

int ac_ai_evaluate_development(const AcPosition *state, AcColor color, int phase) {
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
    homeRow = (color == AC_WHITE) ? 7 : 0;

    for (col = 0; col < AC_COLS; ++col) {
        AcPiece piece = ac_get_piece(&state->board, ac_create_position(homeRow, col));

        if (piece.color != color) {
            continue;
        }

        // penalty for undeveloped knights and bishops
        if (piece.type == AC_KNIGHT || piece.type == AC_BISHOP) {
            ++undevelopedMinors;
            score -= 12;
        }
    }

    // penalty for queen when there are 2 or more undeveloped minors
    if (undevelopedMinors >= 2) {
        AcPiece queen = ac_get_piece(&state->board, ac_create_position(homeRow, 4));

        if (queen.type != AC_QUEEN || queen.color != color) {
            score -= 10;
        }
    }

    // bonus for castling
    if (ac_get_piece(&state->board, ac_create_position(homeRow, 3)).type == AC_KING &&
        ac_get_piece(&state->board, ac_create_position(homeRow, 3)).color == color) {
        score += 8;
    }
    if (ac_get_piece(&state->board, ac_create_position(homeRow, 7)).type == AC_KING &&
        ac_get_piece(&state->board, ac_create_position(homeRow, 7)).color == color) {
        score += 8;
    }

    return score;
}

int ac_ai_evaluate_anteater_threats(const AcBoard *board, AcColor color) {
    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = board->cells[row][col];
            int direction;

            if (piece.type != AC_ANTEATER || piece.color != color) {
                continue;
            }

            for (direction = 0; direction < 4; ++direction) {
                int r2 = row + dr[direction];
                int c2 = col + dc[direction];
                int chainLength;

                chainLength = 0;
                while (r2 >= 0 && r2 < AC_ROWS && c2 >= 0 && c2 < AC_COLS) {
                    AcPiece target = board->cells[r2][c2];

                    if (target.type == AC_ANT && target.color != color) {
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

int ac_ai_evaluate_outposts(const AcBoard *board, AcColor color, int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare pos = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(board, pos);
            int bonus;

            if (piece.color != color) {
                continue;
            }
            if (piece.type != AC_KNIGHT && piece.type != AC_BISHOP && piece.type != AC_ANTEATER) {
                continue;
            }
            if ((color == AC_WHITE && row >= 5) || (color == AC_BLACK && row <= 2)) {
                continue;
            }
            if (!ac_ai_ant_supports_square(board, color, pos)) {
                continue;
            }
            if (ac_ai_enemy_ant_can_attack_square(board, color, pos)) {
                continue;
            }

            if (piece.type == AC_KNIGHT) {
                bonus = 16;
            } else if (piece.type == AC_BISHOP) {
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

int ac_ai_evaluate_absolute(const AcPosition *state) {
    int antFiles[2][AC_COLS];
    int bishopCount[2];
    AcSquare kingPos[2];
    int phase;
    int score;
    int row;
    int col;

    // init
    memset(antFiles, 0, sizeof(antFiles));
    memset(bishopCount, 0, sizeof(bishopCount));
    kingPos[AC_WHITE] = ac_create_position(-1, -1);
    kingPos[AC_BLACK] = ac_create_position(-1, -1);
    phase = 0;
    score = 0;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));

            if (piece.type == AC_EMPTY_PIECE) {
                continue;
            }

            if (piece.type == AC_ANT) {
                ++antFiles[piece.color][col];
            }
            if (piece.type == AC_BISHOP) {
                ++bishopCount[piece.color];
            }
            if (piece.type == AC_KING) {
                kingPos[piece.color] = ac_create_position(row, col);
            }

            phase += ac_ai_phase_value(piece.type);
        }
    }

    // robust
    if (phase > AI_MAX_PHASE) {
        phase = AI_MAX_PHASE;
    }

    // ac_ai_piece_value + PST + mobility
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare pos = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(&state->board, pos);
            int value;

            if (piece.type == AC_EMPTY_PIECE) {
                continue;
            }

            value = ac_ai_piece_value(piece.type);
            value += ac_ai_pst_bonus(piece, row, col, phase);
            if (piece.type == AC_KNIGHT || piece.type == AC_ANTEATER) {
                value += ac_ai_piece_mobility(&state->board, pos, piece) * ac_ai_mobility_weight(piece.type);
            }

            if (piece.color == AC_WHITE) {
                score += value;
            } else {
                score -= value;
            }
        }
    }

    // bishop pair
    if (bishopCount[AC_WHITE] >= 2) {
        score += 36;
    }
    if (bishopCount[AC_BLACK] >= 2) {
        score -= 36;
    }

    score += ac_ai_evaluate_pawns(state, AC_WHITE, antFiles, phase);
    score -= ac_ai_evaluate_pawns(state, AC_BLACK, antFiles, phase);
    score += ac_ai_evaluate_rook_and_queen_files(state, AC_WHITE, antFiles, phase);
    score -= ac_ai_evaluate_rook_and_queen_files(state, AC_BLACK, antFiles, phase);
    score += ac_ai_evaluate_king_safety(state, AC_WHITE, kingPos[AC_WHITE], phase);
    score -= ac_ai_evaluate_king_safety(state, AC_BLACK, kingPos[AC_BLACK], phase);
    score += ac_ai_evaluate_development(state, AC_WHITE, phase);
    score -= ac_ai_evaluate_development(state, AC_BLACK, phase);
    score += ac_ai_evaluate_anteater_threats(&state->board, AC_WHITE);
    score -= ac_ai_evaluate_anteater_threats(&state->board, AC_BLACK);

    // outpost limitation
    if (phase >= 10) {
        score += ac_ai_evaluate_outposts(&state->board, AC_WHITE, phase);
        score -= ac_ai_evaluate_outposts(&state->board, AC_BLACK, phase);
    }

    return score;
}

int ac_ai_evaluate_relative(const AcPosition *state) {
    int absoluteScore;

    absoluteScore = ac_ai_evaluate_absolute(state);
    // tempo bonus
    if (state->currentTurn == AC_WHITE) {
        return absoluteScore + 1;
    }

    return -absoluteScore + 1;
}
