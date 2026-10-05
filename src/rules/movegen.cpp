#include "internal.hpp"

#include <stddef.h>

namespace ac {

/*
 * Alignment assumptions for future extensions:
 * - This file generates move candidates for gameplay and keeps special-move
 *   semantics aligned across validation, FSM resolution, and AC_AI search.
 * - generate_moves() remains pseudo-legal; generate_legal_moves*() are the
 *   single source of truth for fully legal moves.
 * - Legal move generation excludes direct king captures; attack detection for
 *   castling uses local square-attack helpers instead.
 */

/* Centralize coordinate math so every generator rejects out-of-bounds targets
 * before touching board accessors. */
static int position_is_reachable(Square pos) {
    return is_valid_position(pos);
}

/* Return the non-negative magnitude of one integer delta. */
static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

/* Append one generated candidate and normalize the add_move return contract. */
static int add_candidate_move(MoveList *list, Move move) {
    return add_move(list, move) == Status::Ok;
}

/* Check whether the landing square holds any opposing piece. */
static int is_enemy_piece(Piece mover, Piece target) {
    return target.type != PieceType::Empty && target.color != mover.color;
}

/* Kings cannot be captured directly, so legal move generation must exclude
 * king squares from ordinary capture candidates. */
static int is_capturable_enemy_piece(Piece mover, Piece target) {
    return is_enemy_piece(mover, target) && target.type != PieceType::King;
}

/* Check whether the landing square is occupied by the moving side. */
static int is_friendly_piece(Piece mover, Piece target) {
    return target.type != PieceType::Empty && target.color == mover.color;
}

/* Identify the only rank where an ant may attempt its opening double-step. */
static int is_starting_ant_row(Piece piece, Square from) {
    return (piece.color == Color::White && from.row == 6) || (piece.color == Color::Black && from.row == 1);
}

/* Promotion happens only when an ant reaches the opponent home rank. */
static int is_promotion_square(Piece piece, Square to) {
    return piece.type == PieceType::Ant &&
           ((piece.color == Color::White && to.row == 0) || (piece.color == Color::Black && to.row == 7));
}

/* Duplicate one base ant move into the four supported promotion choices. */
static void append_promotion_moves(MoveList *list, Move baseMove) {
    static const SpecialMove promotionTypes[] = {SpecialMove::PromotionQueen, SpecialMove::PromotionRook,
                                                 SpecialMove::PromotionBishop, SpecialMove::PromotionKnight};
    int index;

    for (index = 0; index < (int)(sizeof(promotionTypes) / sizeof(promotionTypes[0])); ++index) {
        Move promotedMove = baseMove;

        set_special_move(&promotedMove, promotionTypes[index]);
        add_candidate_move(list, promotedMove);
    }
}

/* Sliding attack checks must stop at the first blocker between attacker and
 * target because move generation excludes direct king captures. */
static int is_path_clear_for_attack(const Board *board, Square from, Square to) {
    int rowStep;
    int colStep;
    Square current;

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
    while (!position_equal(current, to)) {
        if (get_piece(board, current).type != PieceType::Empty) {
            return 0;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 1;
}

/* Ants attack only on their forward diagonals. */
static int ant_attacks_square(Square from, Piece piece, Square target) {
    int direction;

    direction = (piece.color == Color::White) ? -1 : 1;
    return (target.row - from.row) == direction && absolute_value(target.col - from.col) == 1;
}

/* Rooks attack along ranks and files when no blocker stands in between. */
static int rook_attacks_square(const Board *board, Square from, Square target) {
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    return is_path_clear_for_attack(board, from, target);
}

/* Bishops attack along diagonals when no blocker stands in between. */
static int bishop_attacks_square(const Board *board, Square from, Square target) {
    if (absolute_value(target.row - from.row) != absolute_value(target.col - from.col)) {
        return 0;
    }

    return is_path_clear_for_attack(board, from, target);
}

/* Knights attack in an L-shape and ignore blockers. */
static int knight_attacks_square(Square from, Square target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1) || (rowDistance == 1 && colDistance == 2);
}

/* Kings attack adjacent squares even though legal move generation will not
 * emit direct king captures. */
static int king_attacks_square(Square from, Square target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !position_equal(from, target);
}

/* Anteaters do not attack kings under this ruleset, so they never block
 * castling through attack checks. */
static int piece_attacks_square(const Board *board, Square from, Piece piece, Square target) {
    switch (piece.type) {
    case PieceType::Ant:
        return ant_attacks_square(from, piece, target);
    case PieceType::Rook:
        return rook_attacks_square(board, from, target);
    case PieceType::Knight:
        return knight_attacks_square(from, target);
    case PieceType::Bishop:
        return bishop_attacks_square(board, from, target);
    case PieceType::Queen:
        return rook_attacks_square(board, from, target) || bishop_attacks_square(board, from, target);
    case PieceType::King:
        return king_attacks_square(from, target);
    case PieceType::Anteater:
    case PieceType::Empty:
    default:
        return 0;
    }
}

/* Castling only cares whether one square is attacked by the opposing side. */
int square_attacked(const Board *board, Square target, Color attackingColor) {
    int row;
    int col;

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Square from = create_position(row, col);
            Piece piece = get_piece(board, from);

            if (piece.type == PieceType::Empty || piece.color != attackingColor) {
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
static int history_touches_square(const Position *s, Square q) {
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
static int last_move_is_double_ant_push(const Position *s, Move *out) {
    if (!s || !is_valid_position(s->enPassant))
        return 0;
    Piece p = get_piece(&s->board, s->enPassant);
    if (p.type != PieceType::Ant)
        return 0;
    Square from = s->enPassant;
    from.row += p.color == Color::White ? 2 : -2;
    *out = create_move(from, s->enPassant, p);
    return 1;
}

/* En passant is derived entirely from the latest double ant push plus the
 * current board state. */
static void append_en_passant_moves(const Position *state, Square from, Piece piece, MoveList *list) {
    Move lastMove;
    Square capturePos;
    Square to;
    Piece capturedPiece;
    int direction;

    if (state == NULL || piece.type != PieceType::Ant) {
        return;
    }

    if (!last_move_is_double_ant_push(state, &lastMove) || lastMove.movedPiece.color == piece.color) {
        return;
    }

    capturePos = lastMove.to;
    if (capturePos.row != from.row || absolute_value(capturePos.col - from.col) != 1) {
        return;
    }

    capturedPiece = get_piece(&state->board, capturePos);
    if (capturedPiece.type != PieceType::Ant || capturedPiece.color == piece.color) {
        return;
    }

    direction = (piece.color == Color::White) ? -1 : 1;
    to = create_position(from.row + direction, capturePos.col);
    if (!position_is_reachable(to) || get_piece(&state->board, to).type != PieceType::Empty) {
        return;
    }

    {
        Move move = create_move(from, to, piece);

        add_capture(&move, capturePos, capturedPiece);
        set_special_move(&move, SpecialMove::EnPassant);
        add_candidate_move(list, move);
    }
}

/* Anteater chain capture paths cannot revisit ants that were already eaten
 * earlier in the same move. */
static int move_already_captures_square(const Move *move, Square pos) {
    int index;

    if (move == NULL) {
        return 0;
    }

    for (index = 0; index < move->captureCount; ++index) {
        if (position_equal(move->captures[index].pos, pos)) {
            return 1;
        }
    }

    return 0;
}

/* Anteater chains expose their full eaten route through path[] so explicit
 * validation can distinguish different multi-capture choices. */
static void finalize_anteater_capture_move(Move *move) {
    int index;

    if (move == NULL) {
        return;
    }

    set_special_move(move, SpecialMove::AnteaterCapture);
    move->pathLength = 0;
    if (move->captureCount < 2) {
        return;
    }

    for (index = 0; index < move->captureCount; ++index) {
        add_path_step(move, move->captures[index].pos);
    }
}

/* After the first adjacent ant is eaten, the anteater may continue by eating
 * orthogonally adjacent ants, turning as needed, and stopping on any chosen
 * prefix endpoint. */
static void append_anteater_capture_paths(const Board *board, Square current, Piece piece, Move partialMove,
                                          MoveList *list) {
    static const int rowSteps[] = {-1, 1, 0, 0};
    static const int colSteps[] = {0, 0, -1, 1};
    int index;
    Move emittedMove;

    emittedMove = partialMove;
    emittedMove.to = current;
    finalize_anteater_capture_move(&emittedMove);
    add_candidate_move(list, emittedMove);

    if (partialMove.captureCount >= MaxChain) {
        return;
    }

    for (index = 0; index < 4; ++index) {
        Square next = create_position(current.row + rowSteps[index], current.col + colSteps[index]);
        Piece target;
        Move extendedMove;

        if (!position_is_reachable(next) || move_already_captures_square(&partialMove, next)) {
            continue;
        }

        target = get_piece(board, next);
        if (target.type != PieceType::Ant || target.color == piece.color) {
            continue;
        }

        extendedMove = partialMove;
        add_capture(&extendedMove, next, target);
        append_anteater_capture_paths(board, next, piece, extendedMove, list);
    }
}

/* Sliding pieces all share the same scan pattern: stop at the first occupied
 * square and only keep the capture if that blocker belongs to the opponent. */
static void scan_sliding_direction(const Board *board, Square from, Piece piece, int rowStep, int colStep,
                                   MoveList *list) {
    Square current;

    current = from;
    current.row += rowStep;
    current.col += colStep;

    while (position_is_reachable(current)) {
        Piece target = get_piece(board, current);
        Move move;

        if (is_friendly_piece(piece, target)) {
            break;
        }

        move = create_move(from, current, piece);
        if (is_capturable_enemy_piece(piece, target)) {
            add_capture(&move, current, target);
            add_candidate_move(list, move);
            break;
        }

        if (target.type != PieceType::Empty) {
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
static void generate_ant_moves(const Position *state, Square from, Piece piece, MoveList *list) {
    Square oneStep;
    Square twoStep;
    Square diagonal;
    int direction;
    int colOffset;

    direction = (piece.color == Color::White) ? -1 : 1;

    oneStep = create_position(from.row + direction, from.col);
    if (position_is_reachable(oneStep) && get_piece(&state->board, oneStep).type == PieceType::Empty) {
        Move oneStepMove = create_move(from, oneStep, piece);

        if (is_promotion_square(piece, oneStep)) {
            append_promotion_moves(list, oneStepMove);
        } else {
            add_candidate_move(list, oneStepMove);
        }

        twoStep = create_position(from.row + (2 * direction), from.col);
        if (is_starting_ant_row(piece, from) && position_is_reachable(twoStep) &&
            get_piece(&state->board, twoStep).type == PieceType::Empty) {
            add_candidate_move(list, create_move(from, twoStep, piece));
        }
    }

    for (colOffset = -1; colOffset <= 1; colOffset += 2) {
        Piece target;
        Move move;

        diagonal = create_position(from.row + direction, from.col + colOffset);
        if (!position_is_reachable(diagonal)) {
            continue;
        }

        target = get_piece(&state->board, diagonal);
        if (!is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = create_move(from, diagonal, piece);
        add_capture(&move, diagonal, target);
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
static void generate_anteater_moves(const Board *board, Square from, Piece piece, MoveList *list) {
    static const int rowSteps[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int colSteps[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int index;

    for (index = 0; index < 8; ++index) {
        Square to = create_position(from.row + rowSteps[index], from.col + colSteps[index]);
        Piece target;
        Move move;

        if (!position_is_reachable(to)) {
            continue;
        }

        target = get_piece(board, to);
        if (is_friendly_piece(piece, target)) {
            continue;
        }

        move = create_move(from, to, piece);
        if (target.type == PieceType::Empty) {
            add_candidate_move(list, move);
            continue;
        }

        if (target.type == PieceType::Ant && target.color != piece.color) {
            Move captureMove = move;

            add_capture(&captureMove, to, target);
            append_anteater_capture_paths(board, to, piece, captureMove, list);
        }
    }
}

/* Rooks generate along ranks and files until the first blocker. */
static void generate_rook_moves(const Board *board, Square from, Piece piece, MoveList *list) {
    scan_sliding_direction(board, from, piece, -1, 0, list);
    scan_sliding_direction(board, from, piece, 1, 0, list);
    scan_sliding_direction(board, from, piece, 0, -1, list);
    scan_sliding_direction(board, from, piece, 0, 1, list);
}

/* Bishops generate along the four diagonals until the first blocker. */
static void generate_bishop_moves(const Board *board, Square from, Piece piece, MoveList *list) {
    scan_sliding_direction(board, from, piece, -1, -1, list);
    scan_sliding_direction(board, from, piece, -1, 1, list);
    scan_sliding_direction(board, from, piece, 1, -1, list);
    scan_sliding_direction(board, from, piece, 1, 1, list);
}

/* Queens combine the rook and bishop movement patterns. */
static void generate_queen_moves(const Board *board, Square from, Piece piece, MoveList *list) {
    generate_rook_moves(board, from, piece, list);
    generate_bishop_moves(board, from, piece, list);
}

/* Knights ignore blockers and only care about the landing square. */
static void generate_knight_moves(const Board *board, Square from, Piece piece, MoveList *list) {
    static const int rowOffsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    static const int colOffsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};
    int index;

    for (index = 0; index < 8; ++index) {
        Square to = create_position(from.row + rowOffsets[index], from.col + colOffsets[index]);
        Piece target;
        Move move;

        if (!position_is_reachable(to)) {
            continue;
        }

        target = get_piece(board, to);
        if (is_friendly_piece(piece, target)) {
            continue;
        }

        if (target.type != PieceType::Empty && !is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = create_move(from, to, piece);
        if (is_capturable_enemy_piece(piece, target)) {
            add_capture(&move, to, target);
        }
        add_candidate_move(list, move);
    }
}

/* Kings are single-step movers in any direction, with the same landing rules
 * as other ordinary capture pieces. */
static void generate_king_step_moves(const Board *board, Square from, Piece piece, MoveList *list) {
    int rowOffset;
    int colOffset;

    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            Square to;
            Piece target;
            Move move;

            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            to = create_position(from.row + rowOffset, from.col + colOffset);
            if (!position_is_reachable(to)) {
                continue;
            }

            target = get_piece(board, to);
            if (is_friendly_piece(piece, target)) {
                continue;
            }

            if (target.type != PieceType::Empty && !is_capturable_enemy_piece(piece, target)) {
                continue;
            }

            move = create_move(from, to, piece);
            if (is_capturable_enemy_piece(piece, target)) {
                add_capture(&move, to, target);
            }
            add_candidate_move(list, move);
        }
    }
}

/* Castling rights are reconstructed from history and the current board. */
static void append_castling_moves(const Position *state, Square from, Piece piece, MoveList *list) {
    Square kingStart;
    Square kingsideRookPos;
    Square queensideRookPos;
    Color enemyColor;
    Piece kingsideRook;
    Piece queensideRook;
    int row;

    if (state == NULL || piece.type != PieceType::King) {
        return;
    }

    row = (piece.color == Color::White) ? 7 : 0;
    kingStart = create_position(row, 5);
    if (!position_equal(from, kingStart) || history_touches_square(state, kingStart)) {
        return;
    }

    enemyColor = (piece.color == Color::White) ? Color::Black : Color::White;
    if (is_in_check(state, piece.color) == 1) {
        return;
    }

    kingsideRookPos = create_position(row, 9);
    kingsideRook = get_piece(&state->board, kingsideRookPos);
    if (kingsideRook.type == PieceType::Rook && kingsideRook.color == piece.color &&
        history_touches_square(state, kingsideRookPos) == 0 &&
        get_piece(&state->board, create_position(row, 6)).type == PieceType::Empty &&
        get_piece(&state->board, create_position(row, 7)).type == PieceType::Empty &&
        get_piece(&state->board, create_position(row, 8)).type == PieceType::Empty &&
        square_attacked(&state->board, create_position(row, 6), enemyColor) == 0 &&
        square_attacked(&state->board, create_position(row, 7), enemyColor) == 0) {
        Move move = create_move(from, create_position(row, 7), piece);

        set_special_move(&move, SpecialMove::CastlingKingside);
        add_candidate_move(list, move);
    }

    queensideRookPos = create_position(row, 0);
    queensideRook = get_piece(&state->board, queensideRookPos);
    if (queensideRook.type == PieceType::Rook && queensideRook.color == piece.color &&
        history_touches_square(state, queensideRookPos) == 0 &&
        get_piece(&state->board, create_position(row, 1)).type == PieceType::Empty &&
        get_piece(&state->board, create_position(row, 2)).type == PieceType::Empty &&
        get_piece(&state->board, create_position(row, 3)).type == PieceType::Empty &&
        get_piece(&state->board, create_position(row, 4)).type == PieceType::Empty &&
        square_attacked(&state->board, create_position(row, 4), enemyColor) == 0 &&
        square_attacked(&state->board, create_position(row, 3), enemyColor) == 0) {
        Move move = create_move(from, create_position(row, 3), piece);

        set_special_move(&move, SpecialMove::CastlingQueenside);
        add_candidate_move(list, move);
    }
}

/* Dispatch piece-specific generation without exposing helper functions publicly. */
static void generate_pseudo_moves_for_position(const Position *state, Square from, Piece piece, MoveList *list) {
    switch (piece.type) {
    case PieceType::Ant:
        generate_ant_moves(state, from, piece, list);
        break;
    case PieceType::Anteater:
        generate_anteater_moves(&state->board, from, piece, list);
        break;
    case PieceType::Rook:
        generate_rook_moves(&state->board, from, piece, list);
        break;
    case PieceType::Bishop:
        generate_bishop_moves(&state->board, from, piece, list);
        break;
    case PieceType::Queen:
        generate_queen_moves(&state->board, from, piece, list);
        break;
    case PieceType::Knight:
        generate_knight_moves(&state->board, from, piece, list);
        break;
    case PieceType::King:
        generate_king_step_moves(&state->board, from, piece, list);
        append_castling_moves(state, from, piece, list);
        break;
    case PieceType::Empty:
    default:
        break;
    }
}

/* Reject empty, off-turn, and out-of-bounds origins before generation begins. */
static int can_generate_from_position(const Position *state, Square from, Piece *pieceOut) {
    Piece piece;

    if (state == NULL || !position_is_reachable(from)) {
        return 0;
    }

    piece = get_piece(&state->board, from);
    if (piece.type == PieceType::Empty || piece.color != state->currentTurn) {
        return 0;
    }

    if (pieceOut != NULL) {
        *pieceOut = piece;
    }

    return 1;
}

/* Filter pseudo-legal moves down to moves that leave the moving king safe. */
static Status filter_legal_moves(const Position *s, MoveList *list) {
    int count = list->count, out = 0;
    if (list->status != Status::Ok)
        return list->status;
    for (int i = 0; i < count; ++i) {
        Position next = *s;
        Undo undo;
        if (position_make(&next, list->moves[i], &undo) != Status::Ok)
            continue;
        if (!is_in_check(&next, s->currentTurn))
            list->moves[out++] = list->moves[i];
    }
    list->count = out;
    return Status::Ok;
}

/* Generate every pseudo-legal candidate move for the side whose turn is stored
 * in state. */
Status generate_moves(const Position *state, MoveList *list) {
    int row;
    int col;

    if (state == NULL || list == NULL) {
        return Status::InvalidArgument;
    }

    init_move_list(list);
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece;
            Square from = create_position(row, col);

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
Status generate_legal_moves(const Position *s, MoveList *list) {
    if (!s || !list)
        return Status::InvalidArgument;
    Status status = generate_moves(s, list);
    return status != Status::Ok ? status : filter_legal_moves(s, list);
}

/* Generate legal moves for one origin square if it belongs to the side to
 * move. */
Status generate_legal_moves_for_position(const Position *s, Square from, MoveList *list) {
    Piece piece;
    if (!s || !list)
        return Status::InvalidArgument;
    init_move_list(list);
    if (!can_generate_from_position(s, from, &piece))
        return Status::Ok;
    generate_pseudo_moves_for_position(s, from, piece, list);
    return filter_legal_moves(s, list);
}

} // namespace ac
