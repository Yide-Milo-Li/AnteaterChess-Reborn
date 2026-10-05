#include "internal.h"

#include <stddef.h>

/*
 * Alignment assumptions for future extensions:
 * - This file generates move candidates for gameplay and keeps special-move
 *   semantics aligned across validation, FSM resolution, and AC_AI search.
 * - ac_generate_moves() remains pseudo-legal; ac_generate_legal_moves*() are the
 *   single source of truth for fully legal moves.
 * - Legal move generation excludes direct king captures; attack detection for
 *   castling uses local square-attack helpers instead.
 */

/* Centralize coordinate math so every generator rejects out-of-bounds targets
 * before touching board accessors. */
static int position_is_reachable(AcSquare pos) {
    return ac_is_valid_position(pos);
}

/* Return the non-negative magnitude of one integer delta. */
static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

/* Append one generated candidate and normalize the ac_add_move return contract. */
static int add_candidate_move(AcMoveList *list, AcMove move) {
    return ac_add_move(list, move) == 0;
}

/* Check whether the landing square holds any opposing piece. */
static int is_enemy_piece(AcPiece mover, AcPiece target) {
    return target.type != AC_EMPTY_PIECE && target.color != mover.color;
}

/* Kings cannot be captured directly, so legal move generation must exclude
 * king squares from ordinary capture candidates. */
static int is_capturable_enemy_piece(AcPiece mover, AcPiece target) {
    return is_enemy_piece(mover, target) && target.type != AC_KING;
}

/* Check whether the landing square is occupied by the moving side. */
static int is_friendly_piece(AcPiece mover, AcPiece target) {
    return target.type != AC_EMPTY_PIECE && target.color == mover.color;
}

/* Identify the only rank where an ant may attempt its opening double-step. */
static int is_starting_ant_row(AcPiece piece, AcSquare from) {
    return (piece.color == AC_WHITE && from.row == 6) || (piece.color == AC_BLACK && from.row == 1);
}

/* Promotion happens only when an ant reaches the opponent home rank. */
static int is_promotion_square(AcPiece piece, AcSquare to) {
    return piece.type == AC_ANT &&
           ((piece.color == AC_WHITE && to.row == 0) || (piece.color == AC_BLACK && to.row == 7));
}

/* Duplicate one base ant move into the four supported promotion choices. */
static void append_promotion_moves(AcMoveList *list, AcMove baseMove) {
    static const AcSpecialMove promotionTypes[] = {AC_PROMOTION_QUEEN, AC_PROMOTION_ROOK, AC_PROMOTION_BISHOP,
                                                   AC_PROMOTION_KNIGHT};
    int index;

    for (index = 0; index < (int)(sizeof(promotionTypes) / sizeof(promotionTypes[0])); ++index) {
        AcMove promotedMove = baseMove;

        ac_set_special_move(&promotedMove, promotionTypes[index]);
        add_candidate_move(list, promotedMove);
    }
}

/* Sliding attack checks must stop at the first blocker between attacker and
 * target because move generation excludes direct king captures. */
static int is_path_clear_for_attack(const AcBoard *board, AcSquare from, AcSquare to) {
    int rowStep;
    int colStep;
    AcSquare current;

    rowStep = 0;
    colStep = 0;
    if (to.row > from.row) {
        rowStep = 1;
    } else if (to.row < from.row) {
        rowStep = -1;
    }

    if (to.col > from.col) {
        colStep = 1;
    } else if (to.col < from.col) {
        colStep = -1;
    }

    current = from;
    current.row += rowStep;
    current.col += colStep;
    while (!ac_position_equal(current, to)) {
        if (ac_get_piece(board, current).type != AC_EMPTY_PIECE) {
            return 0;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 1;
}

/* Ants attack only on their forward diagonals. */
static int ant_attacks_square(AcSquare from, AcPiece piece, AcSquare target) {
    int direction;

    direction = (piece.color == AC_WHITE) ? -1 : 1;
    return (target.row - from.row) == direction && absolute_value(target.col - from.col) == 1;
}

/* Rooks attack along ranks and files when no blocker stands in between. */
static int rook_attacks_square(const AcBoard *board, AcSquare from, AcSquare target) {
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    return is_path_clear_for_attack(board, from, target);
}

/* Bishops attack along diagonals when no blocker stands in between. */
static int bishop_attacks_square(const AcBoard *board, AcSquare from, AcSquare target) {
    if (absolute_value(target.row - from.row) != absolute_value(target.col - from.col)) {
        return 0;
    }

    return is_path_clear_for_attack(board, from, target);
}

/* Knights attack in an L-shape and ignore blockers. */
static int knight_attacks_square(AcSquare from, AcSquare target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
}

/* Kings attack adjacent squares even though legal move generation will not
 * emit direct king captures. */
static int king_attacks_square(AcSquare from, AcSquare target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !ac_position_equal(from, target);
}

/* Anteaters do not attack kings under this ruleset, so they never block
 * castling through attack checks. */
static int piece_attacks_square(const AcBoard *board, AcSquare from, AcPiece piece, AcSquare target) {
    switch (piece.type) {
    case AC_ANT:
        return ant_attacks_square(from, piece, target);
    case AC_ROOK:
        return rook_attacks_square(board, from, target);
    case AC_KNIGHT:
        return knight_attacks_square(from, target);
    case AC_BISHOP:
        return bishop_attacks_square(board, from, target);
    case AC_QUEEN:
        return rook_attacks_square(board, from, target) || bishop_attacks_square(board, from, target);
    case AC_KING:
        return king_attacks_square(from, target);
    case AC_ANTEATER:
    case AC_EMPTY_PIECE:
    default:
        return 0;
    }
}

/* Castling only cares whether one square is attacked by the opposing side. */
int ac_square_attacked(const AcBoard *board, AcSquare target, AcColor attackingColor) {
    int row;
    int col;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare from = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(board, from);

            if (piece.type == AC_EMPTY_PIECE || piece.color != attackingColor) {
                continue;
            }

            if (piece_attacks_square(board, from, piece, target) == 1) {
                return 1;
            }
        }
    }

    return 0;
}

/* Special-move rights are derived from move history without adding new public
 * state fields. Once a starting square is touched, its original right is gone. */
static int history_touches_square(const AcPosition *s, AcSquare q) {
    int shift = q.row == 7 ? 0 : 2;
    if (q.col == 5)
        return !(s->castlingRights & (3 << shift));
    if (q.col == 9)
        return !(s->castlingRights & (1 << shift));
    if (q.col == 0)
        return !(s->castlingRights & (2 << shift));
    return 0;
}

/* Promotion and en passant both need the exact previous move. */
static int last_move_is_double_ant_push(const AcPosition *s, AcMove *out) {
    if (!s || !ac_is_valid_position(s->enPassant))
        return 0;
    AcPiece p = ac_get_piece(&s->board, s->enPassant);
    if (p.type != AC_ANT)
        return 0;
    AcSquare from = s->enPassant;
    from.row += p.color == AC_WHITE ? 2 : -2;
    *out = ac_create_move(from, s->enPassant, p);
    return 1;
}

/* En passant is derived entirely from the latest double ant push plus the
 * current board state. */
static void append_en_passant_moves(const AcPosition *state, AcSquare from, AcPiece piece, AcMoveList *list) {
    AcMove lastMove;
    AcSquare capturePos;
    AcSquare to;
    AcPiece capturedPiece;
    int direction;

    if (state == NULL || piece.type != AC_ANT) {
        return;
    }

    if (!last_move_is_double_ant_push(state, &lastMove) || lastMove.movedPiece.color == piece.color) {
        return;
    }

    capturePos = lastMove.to;
    if (capturePos.row != from.row || absolute_value(capturePos.col - from.col) != 1) {
        return;
    }

    capturedPiece = ac_get_piece(&state->board, capturePos);
    if (capturedPiece.type != AC_ANT || capturedPiece.color == piece.color) {
        return;
    }

    direction = (piece.color == AC_WHITE) ? -1 : 1;
    to = ac_create_position(from.row + direction, capturePos.col);
    if (!position_is_reachable(to) || ac_get_piece(&state->board, to).type != AC_EMPTY_PIECE) {
        return;
    }

    {
        AcMove move = ac_create_move(from, to, piece);

        ac_add_capture(&move, capturePos, capturedPiece);
        ac_set_special_move(&move, AC_EN_PASSANT);
        add_candidate_move(list, move);
    }
}

/* Anteater chain capture paths cannot revisit ants that were already eaten
 * earlier in the same move. */
static int move_already_captures_square(const AcMove *move, AcSquare pos) {
    int index;

    if (move == NULL) {
        return 0;
    }

    for (index = 0; index < move->captureCount; ++index) {
        if (ac_position_equal(move->captures[index].pos, pos)) {
            return 1;
        }
    }

    return 0;
}

/* Anteater chains expose their full eaten route through path[] so explicit
 * validation can distinguish different multi-capture choices. */
static void finalize_anteater_capture_move(AcMove *move) {
    int index;

    if (move == NULL) {
        return;
    }

    ac_set_special_move(move, AC_ANTEATER_CAPTURE);
    move->pathLength = 0;
    if (move->captureCount < 2) {
        return;
    }

    for (index = 0; index < move->captureCount; ++index) {
        ac_add_path_step(move, move->captures[index].pos);
    }
}

/* After the first adjacent ant is eaten, the anteater may continue by eating
 * orthogonally adjacent ants, turning as needed, and stopping on any chosen
 * prefix endpoint. */
static void append_anteater_capture_paths(const AcBoard *board, AcSquare current, AcPiece piece, AcMove partialMove,
                                          AcMoveList *list) {
    static const int rowSteps[] = {-1, 1, 0, 0};
    static const int colSteps[] = {0, 0, -1, 1};
    int index;
    AcMove emittedMove;

    emittedMove = partialMove;
    emittedMove.to = current;
    finalize_anteater_capture_move(&emittedMove);
    add_candidate_move(list, emittedMove);

    if (partialMove.captureCount >= AC_MAX_CHAIN) {
        return;
    }

    for (index = 0; index < 4; ++index) {
        AcSquare next = ac_create_position(current.row + rowSteps[index], current.col + colSteps[index]);
        AcPiece target;
        AcMove extendedMove;

        if (!position_is_reachable(next) || move_already_captures_square(&partialMove, next)) {
            continue;
        }

        target = ac_get_piece(board, next);
        if (target.type != AC_ANT || target.color == piece.color) {
            continue;
        }

        extendedMove = partialMove;
        ac_add_capture(&extendedMove, next, target);
        append_anteater_capture_paths(board, next, piece, extendedMove, list);
    }
}

/* Sliding pieces all share the same scan pattern: stop at the first occupied
 * square and only keep the capture if that blocker belongs to the opponent. */
static void scan_sliding_direction(const AcBoard *board, AcSquare from, AcPiece piece, int rowStep, int colStep,
                                   AcMoveList *list) {
    AcSquare current;

    current = from;
    current.row += rowStep;
    current.col += colStep;

    while (position_is_reachable(current)) {
        AcPiece target = ac_get_piece(board, current);
        AcMove move;

        if (is_friendly_piece(piece, target)) {
            break;
        }

        move = ac_create_move(from, current, piece);
        if (is_capturable_enemy_piece(piece, target)) {
            ac_add_capture(&move, current, target);
            add_candidate_move(list, move);
            break;
        }

        if (target.type != AC_EMPTY_PIECE) {
            break;
        }

        if (!add_candidate_move(list, move)) {
            break;
        }

        current.row += rowStep;
        current.col += colStep;
    }
}

/* Ants move forward into empty squares, but capture only on the forward
 * diagonals. Promotions fork into four variants; en passant depends on the
 * immediately preceding double-step ant move. */
static void generate_ant_moves(const AcPosition *state, AcSquare from, AcPiece piece, AcMoveList *list) {
    AcSquare oneStep;
    AcSquare twoStep;
    AcSquare diagonal;
    int direction;
    int colOffset;

    direction = (piece.color == AC_WHITE) ? -1 : 1;

    oneStep = ac_create_position(from.row + direction, from.col);
    if (position_is_reachable(oneStep) && ac_get_piece(&state->board, oneStep).type == AC_EMPTY_PIECE) {
        AcMove oneStepMove = ac_create_move(from, oneStep, piece);

        if (is_promotion_square(piece, oneStep)) {
            append_promotion_moves(list, oneStepMove);
        } else {
            add_candidate_move(list, oneStepMove);
        }

        twoStep = ac_create_position(from.row + (2 * direction), from.col);
        if (is_starting_ant_row(piece, from) && position_is_reachable(twoStep) &&
            ac_get_piece(&state->board, twoStep).type == AC_EMPTY_PIECE) {
            add_candidate_move(list, ac_create_move(from, twoStep, piece));
        }
    }

    for (colOffset = -1; colOffset <= 1; colOffset += 2) {
        AcPiece target;
        AcMove move;

        diagonal = ac_create_position(from.row + direction, from.col + colOffset);
        if (!position_is_reachable(diagonal)) {
            continue;
        }

        target = ac_get_piece(&state->board, diagonal);
        if (!is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = ac_create_move(from, diagonal, piece);
        ac_add_capture(&move, diagonal, target);
        if (is_promotion_square(piece, diagonal)) {
            append_promotion_moves(list, move);
        } else {
            add_candidate_move(list, move);
        }
    }

    append_en_passant_moves(state, from, piece, list);
}

/* Anteaters can move one square in any direction, but they only capture ants.
 * A capture may start on any adjacent ant, then continue recursively through
 * orthogonally adjacent ants, turning as needed, and stopping on any prefix. */
static void generate_anteater_moves(const AcBoard *board, AcSquare from, AcPiece piece, AcMoveList *list) {
    static const int rowSteps[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int colSteps[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int index;

    for (index = 0; index < 8; ++index) {
        AcSquare to = ac_create_position(from.row + rowSteps[index], from.col + colSteps[index]);
        AcPiece target;
        AcMove move;

        if (!position_is_reachable(to)) {
            continue;
        }

        target = ac_get_piece(board, to);
        if (is_friendly_piece(piece, target)) {
            continue;
        }

        move = ac_create_move(from, to, piece);
        if (target.type == AC_EMPTY_PIECE) {
            add_candidate_move(list, move);
            continue;
        }

        if (target.type == AC_ANT && target.color != piece.color) {
            AcMove captureMove = move;

            ac_add_capture(&captureMove, to, target);
            append_anteater_capture_paths(board, to, piece, captureMove, list);
        }
    }
}

/* Rooks generate along ranks and files until the first blocker. */
static void generate_rook_moves(const AcBoard *board, AcSquare from, AcPiece piece, AcMoveList *list) {
    scan_sliding_direction(board, from, piece, -1, 0, list);
    scan_sliding_direction(board, from, piece, 1, 0, list);
    scan_sliding_direction(board, from, piece, 0, -1, list);
    scan_sliding_direction(board, from, piece, 0, 1, list);
}

/* Bishops generate along the four diagonals until the first blocker. */
static void generate_bishop_moves(const AcBoard *board, AcSquare from, AcPiece piece, AcMoveList *list) {
    scan_sliding_direction(board, from, piece, -1, -1, list);
    scan_sliding_direction(board, from, piece, -1, 1, list);
    scan_sliding_direction(board, from, piece, 1, -1, list);
    scan_sliding_direction(board, from, piece, 1, 1, list);
}

/* Queens combine the rook and bishop movement patterns. */
static void generate_queen_moves(const AcBoard *board, AcSquare from, AcPiece piece, AcMoveList *list) {
    generate_rook_moves(board, from, piece, list);
    generate_bishop_moves(board, from, piece, list);
}

/* Knights ignore blockers and only care about the landing square. */
static void generate_knight_moves(const AcBoard *board, AcSquare from, AcPiece piece, AcMoveList *list) {
    static const int rowOffsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    static const int colOffsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};
    int index;

    for (index = 0; index < 8; ++index) {
        AcSquare to = ac_create_position(from.row + rowOffsets[index], from.col + colOffsets[index]);
        AcPiece target;
        AcMove move;

        if (!position_is_reachable(to)) {
            continue;
        }

        target = ac_get_piece(board, to);
        if (is_friendly_piece(piece, target)) {
            continue;
        }

        if (target.type != AC_EMPTY_PIECE && !is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = ac_create_move(from, to, piece);
        if (is_capturable_enemy_piece(piece, target)) {
            ac_add_capture(&move, to, target);
        }
        add_candidate_move(list, move);
    }
}

/* Kings are single-step movers in any direction, with the same landing rules
 * as other ordinary capture pieces. */
static void generate_king_step_moves(const AcBoard *board, AcSquare from, AcPiece piece, AcMoveList *list) {
    int rowOffset;
    int colOffset;

    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            AcSquare to;
            AcPiece target;
            AcMove move;

            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            to = ac_create_position(from.row + rowOffset, from.col + colOffset);
            if (!position_is_reachable(to)) {
                continue;
            }

            target = ac_get_piece(board, to);
            if (is_friendly_piece(piece, target)) {
                continue;
            }

            if (target.type != AC_EMPTY_PIECE && !is_capturable_enemy_piece(piece, target)) {
                continue;
            }

            move = ac_create_move(from, to, piece);
            if (is_capturable_enemy_piece(piece, target)) {
                ac_add_capture(&move, to, target);
            }
            add_candidate_move(list, move);
        }
    }
}

/* Castling rights are reconstructed from history and the current board. */
static void append_castling_moves(const AcPosition *state, AcSquare from, AcPiece piece, AcMoveList *list) {
    AcSquare kingStart;
    AcSquare kingsideRookPos;
    AcSquare queensideRookPos;
    AcColor enemyColor;
    AcPiece kingsideRook;
    AcPiece queensideRook;
    int row;

    if (state == NULL || piece.type != AC_KING) {
        return;
    }

    row = (piece.color == AC_WHITE) ? 7 : 0;
    kingStart = ac_create_position(row, 5);
    if (!ac_position_equal(from, kingStart) || history_touches_square(state, kingStart)) {
        return;
    }

    enemyColor = (piece.color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    if (ac_is_in_check(state, piece.color) == 1) {
        return;
    }

    kingsideRookPos = ac_create_position(row, 9);
    kingsideRook = ac_get_piece(&state->board, kingsideRookPos);
    if (kingsideRook.type == AC_ROOK && kingsideRook.color == piece.color &&
        history_touches_square(state, kingsideRookPos) == 0 &&
        ac_get_piece(&state->board, ac_create_position(row, 6)).type == AC_EMPTY_PIECE &&
        ac_get_piece(&state->board, ac_create_position(row, 7)).type == AC_EMPTY_PIECE &&
        ac_get_piece(&state->board, ac_create_position(row, 8)).type == AC_EMPTY_PIECE &&
        ac_square_attacked(&state->board, ac_create_position(row, 6), enemyColor) == 0 &&
        ac_square_attacked(&state->board, ac_create_position(row, 7), enemyColor) == 0) {
        AcMove move = ac_create_move(from, ac_create_position(row, 7), piece);

        ac_set_special_move(&move, AC_CASTLING_KINGSIDE);
        add_candidate_move(list, move);
    }

    queensideRookPos = ac_create_position(row, 0);
    queensideRook = ac_get_piece(&state->board, queensideRookPos);
    if (queensideRook.type == AC_ROOK && queensideRook.color == piece.color &&
        history_touches_square(state, queensideRookPos) == 0 &&
        ac_get_piece(&state->board, ac_create_position(row, 1)).type == AC_EMPTY_PIECE &&
        ac_get_piece(&state->board, ac_create_position(row, 2)).type == AC_EMPTY_PIECE &&
        ac_get_piece(&state->board, ac_create_position(row, 3)).type == AC_EMPTY_PIECE &&
        ac_get_piece(&state->board, ac_create_position(row, 4)).type == AC_EMPTY_PIECE &&
        ac_square_attacked(&state->board, ac_create_position(row, 4), enemyColor) == 0 &&
        ac_square_attacked(&state->board, ac_create_position(row, 3), enemyColor) == 0) {
        AcMove move = ac_create_move(from, ac_create_position(row, 3), piece);

        ac_set_special_move(&move, AC_CASTLING_QUEENSIDE);
        add_candidate_move(list, move);
    }
}

/* Dispatch piece-specific generation without exposing helper functions publicly. */
static void generate_pseudo_moves_for_position(const AcPosition *state, AcSquare from, AcPiece piece,
                                               AcMoveList *list) {
    switch (piece.type) {
    case AC_ANT:
        generate_ant_moves(state, from, piece, list);
        break;
    case AC_ANTEATER:
        generate_anteater_moves(&state->board, from, piece, list);
        break;
    case AC_ROOK:
        generate_rook_moves(&state->board, from, piece, list);
        break;
    case AC_BISHOP:
        generate_bishop_moves(&state->board, from, piece, list);
        break;
    case AC_QUEEN:
        generate_queen_moves(&state->board, from, piece, list);
        break;
    case AC_KNIGHT:
        generate_knight_moves(&state->board, from, piece, list);
        break;
    case AC_KING:
        generate_king_step_moves(&state->board, from, piece, list);
        append_castling_moves(state, from, piece, list);
        break;
    case AC_EMPTY_PIECE:
    default:
        break;
    }
}

/* Reject empty, off-turn, and out-of-bounds origins before generation begins. */
static int can_generate_from_position(const AcPosition *state, AcSquare from, AcPiece *pieceOut) {
    AcPiece piece;

    if (state == NULL || !position_is_reachable(from)) {
        return 0;
    }

    piece = ac_get_piece(&state->board, from);
    if (piece.type == AC_EMPTY_PIECE || piece.color != state->currentTurn) {
        return 0;
    }

    if (pieceOut != NULL) {
        *pieceOut = piece;
    }

    return 1;
}

/* Filter pseudo-legal moves down to moves that leave the moving king safe. */
static int filter_legal_moves(const AcPosition *s, AcMoveList *list) {
    int count = list->count, out = 0;
    if (list->status != AC_OK)
        return list->status;
    for (int i = 0; i < count; ++i) {
        AcPosition next = *s;
        AcUndo undo;
        if (ac_position_make(&next, list->moves[i], &undo) != AC_OK)
            continue;
        if (!ac_is_in_check(&next, s->currentTurn))
            list->moves[out++] = list->moves[i];
    }
    list->count = out;
    return AC_OK;
}

/* Generate every pseudo-legal candidate move for the side whose turn is stored
 * in state. */
int ac_generate_moves(const AcPosition *state, AcMoveList *list) {
    int row;
    int col;

    if (state == NULL || list == NULL) {
        return 1;
    }

    ac_init_move_list(list);
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece;
            AcSquare from = ac_create_position(row, col);

            if (!can_generate_from_position(state, from, &piece)) {
                continue;
            }

            generate_pseudo_moves_for_position(state, from, piece, list);
        }
    }

    return list->status;
}

/* The legal move API filters pseudo-legal candidates through self-check
 * detection. */
int ac_generate_legal_moves(const AcPosition *s, AcMoveList *list) {
    if (!s || !list)
        return AC_INVALID_ARGUMENT;
    int status = ac_generate_moves(s, list);
    return status ? status : filter_legal_moves(s, list);
}

/* Generate legal moves for one origin square if it belongs to the side to
 * move. */
int ac_generate_legal_moves_for_position(const AcPosition *s, AcSquare from, AcMoveList *list) {
    AcPiece piece;
    if (!s || !list)
        return AC_INVALID_ARGUMENT;
    ac_init_move_list(list);
    if (!can_generate_from_position(s, from, &piece))
        return AC_OK;
    generate_pseudo_moves_for_position(s, from, piece, list);
    return filter_legal_moves(s, list);
}
