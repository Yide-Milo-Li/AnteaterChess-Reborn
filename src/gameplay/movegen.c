#include "gameplay/movegen.h"

#include <stddef.h>

#include "core/board.h"
#include "core/move.h"
#include "core/piece.h"
#include "core/position.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"

/*
 * Alignment assumptions for future extensions:
 * - This file generates move candidates for gameplay and keeps special-move
 *   semantics aligned across validation, FSM resolution, and AI search.
 * - generateMoves() remains pseudo-legal; generateLegalMoves*() are the
 *   single source of truth for fully legal moves.
 * - Legal move generation excludes direct king captures; attack detection for
 *   castling uses local square-attack helpers instead.
 */

/* Centralize coordinate math so every generator rejects out-of-bounds targets
 * before touching board accessors. */
static int position_is_reachable(Position pos) {
    return isValidPosition(pos);
}

/* Return the non-negative magnitude of one integer delta. */
static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

/* Append one generated candidate and normalize the addMove return contract. */
static int add_candidate_move(MoveList *list, Move move) {
    return addMove(list, move) == 0;
}

/* Check whether the landing square holds any opposing piece. */
static int is_enemy_piece(Piece mover, Piece target) {
    return target.type != EMPTY_PIECE && target.color != mover.color;
}

/* Kings cannot be captured directly, so legal move generation must exclude
 * king squares from ordinary capture candidates. */
static int is_capturable_enemy_piece(Piece mover, Piece target) {
    return is_enemy_piece(mover, target) && target.type != KING;
}

/* Check whether the landing square is occupied by the moving side. */
static int is_friendly_piece(Piece mover, Piece target) {
    return target.type != EMPTY_PIECE && target.color == mover.color;
}

/* Identify the only rank where an ant may attempt its opening double-step. */
static int is_starting_ant_row(Piece piece, Position from) {
    return (piece.color == WHITE && from.row == 6)
        || (piece.color == BLACK && from.row == 1);
}

/* Promotion happens only when an ant reaches the opponent home rank. */
static int is_promotion_square(Piece piece, Position to) {
    return piece.type == ANT
        && ((piece.color == WHITE && to.row == 0)
            || (piece.color == BLACK && to.row == 7));
}

/* Duplicate one base ant move into the four supported promotion choices. */
static void append_promotion_moves(MoveList *list, Move baseMove) {
    static const SpecialMove promotionTypes[] = {
        PROMOTION_QUEEN,
        PROMOTION_ROOK,
        PROMOTION_BISHOP,
        PROMOTION_KNIGHT
    };
    int index;

    for (index = 0;
        index < (int)(sizeof(promotionTypes) / sizeof(promotionTypes[0]));
        ++index) {
        Move promotedMove = baseMove;

        setSpecialMove(&promotedMove, promotionTypes[index]);
        add_candidate_move(list, promotedMove);
    }
}

/* Sliding attack checks must stop at the first blocker between attacker and
 * target because move generation excludes direct king captures. */
static int is_path_clear_for_attack(const Board *board, Position from, Position to) {
    int rowStep;
    int colStep;
    Position current;

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
    while (!positionEqual(current, to)) {
        if (getPiece(board, current).type != EMPTY_PIECE) {
            return 0;
        }

        current.row += rowStep;
        current.col += colStep;
    }

    return 1;
}

/* Ants attack only on their forward diagonals. */
static int ant_attacks_square(Position from, Piece piece, Position target) {
    int direction;

    direction = (piece.color == WHITE) ? -1 : 1;
    return (target.row - from.row) == direction
        && absolute_value(target.col - from.col) == 1;
}

/* Rooks attack along ranks and files when no blocker stands in between. */
static int rook_attacks_square(const Board *board, Position from, Position target) {
    if (from.row != target.row && from.col != target.col) {
        return 0;
    }

    return is_path_clear_for_attack(board, from, target);
}

/* Bishops attack along diagonals when no blocker stands in between. */
static int bishop_attacks_square(const Board *board, Position from, Position target) {
    if (absolute_value(target.row - from.row) != absolute_value(target.col - from.col)) {
        return 0;
    }

    return is_path_clear_for_attack(board, from, target);
}

/* Knights attack in an L-shape and ignore blockers. */
static int knight_attacks_square(Position from, Position target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return (rowDistance == 2 && colDistance == 1)
        || (rowDistance == 1 && colDistance == 2);
}

/* Kings attack adjacent squares even though legal move generation will not
 * emit direct king captures. */
static int king_attacks_square(Position from, Position target) {
    int rowDistance;
    int colDistance;

    rowDistance = absolute_value(target.row - from.row);
    colDistance = absolute_value(target.col - from.col);
    return rowDistance <= 1 && colDistance <= 1 && !positionEqual(from, target);
}

/* Anteaters do not attack kings under this ruleset, so they never block
 * castling through attack checks. */
static int piece_attacks_square(const Board *board, Position from, Piece piece, Position target) {
    switch (piece.type) {
        case ANT:
            return ant_attacks_square(from, piece, target);
        case ROOK:
            return rook_attacks_square(board, from, target);
        case KNIGHT:
            return knight_attacks_square(from, target);
        case BISHOP:
            return bishop_attacks_square(board, from, target);
        case QUEEN:
            return rook_attacks_square(board, from, target)
                || bishop_attacks_square(board, from, target);
        case KING:
            return king_attacks_square(from, target);
        case ANTEATER:
        case EMPTY_PIECE:
        default:
            return 0;
    }
}

/* Castling only cares whether one square is attacked by the opposing side. */
static int square_is_attacked(const Board *board, Position target, Color attackingColor) {
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(board, from);

            if (piece.type == EMPTY_PIECE || piece.color != attackingColor) {
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
static int history_touches_square(const GameState *state, Position square) {
    int moveIndex;

    if (state == NULL) {
        return 0;
    }

    for (moveIndex = 0; moveIndex < state->moveHistory.count; ++moveIndex) {
        Move *move = getMove((MoveList *)&state->moveHistory, moveIndex);
        int captureIndex;

        if (move == NULL) {
            continue;
        }

        if (positionEqual(move->from, square) || positionEqual(move->to, square)) {
            return 1;
        }

        for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
            if (positionEqual(move->captures[captureIndex].pos, square)) {
                return 1;
            }
        }
    }

    return 0;
}

/* Promotion and en passant both need the exact previous move. */
static int last_move_is_double_ant_push(const GameState *state, Move *lastMoveOut) {
    Move *lastMove;

    if (state == NULL || state->moveHistory.count <= 0) {
        return 0;
    }

    lastMove = getMove((MoveList *)&state->moveHistory, state->moveHistory.count - 1);
    if (lastMove == NULL) {
        return 0;
    }

    if (lastMove->movedPiece.type != ANT
        || lastMove->from.col != lastMove->to.col
        || absolute_value(lastMove->to.row - lastMove->from.row) != 2) {
        return 0;
    }

    if (lastMoveOut != NULL) {
        *lastMoveOut = *lastMove;
    }

    return 1;
}

/* En passant is derived entirely from the latest double ant push plus the
 * current board state. */
static void append_en_passant_moves(
    const GameState *state,
    Position from,
    Piece piece,
    MoveList *list
) {
    Move lastMove;
    Position capturePos;
    Position to;
    Piece capturedPiece;
    int direction;

    if (state == NULL || piece.type != ANT) {
        return;
    }

    if (!last_move_is_double_ant_push(state, &lastMove)
        || lastMove.movedPiece.color == piece.color) {
        return;
    }

    capturePos = lastMove.to;
    if (capturePos.row != from.row || absolute_value(capturePos.col - from.col) != 1) {
        return;
    }

    capturedPiece = getPiece(&state->board, capturePos);
    if (capturedPiece.type != ANT || capturedPiece.color == piece.color) {
        return;
    }

    direction = (piece.color == WHITE) ? -1 : 1;
    to = createPosition(from.row + direction, capturePos.col);
    if (!position_is_reachable(to) || getPiece(&state->board, to).type != EMPTY_PIECE) {
        return;
    }

    {
        Move move = createMove(from, to, piece);

        addCapture(&move, capturePos, capturedPiece);
        setSpecialMove(&move, EN_PASSANT);
        add_candidate_move(list, move);
    }
}

/* Anteater chain capture paths cannot revisit ants that were already eaten
 * earlier in the same move. */
static int move_already_captures_square(const Move *move, Position pos) {
    int index;

    if (move == NULL) {
        return 0;
    }

    for (index = 0; index < move->captureCount; ++index) {
        if (positionEqual(move->captures[index].pos, pos)) {
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

    setSpecialMove(move, ANTEATER_CAPTURE);
    move->pathLength = 0;
    if (move->captureCount < 2) {
        return;
    }

    for (index = 0; index < move->captureCount; ++index) {
        addPathStep(move, move->captures[index].pos);
    }
}

/* After the first adjacent ant is eaten, the anteater may continue by eating
 * orthogonally adjacent ants, turning as needed, and stopping on any chosen
 * prefix endpoint. */
static void append_anteater_capture_paths(
    const Board *board,
    Position current,
    Piece piece,
    Move partialMove,
    MoveList *list
) {
    static const int rowSteps[] = {-1, 1, 0, 0};
    static const int colSteps[] = {0, 0, -1, 1};
    int index;
    Move emittedMove;

    emittedMove = partialMove;
    emittedMove.to = current;
    finalize_anteater_capture_move(&emittedMove);
    add_candidate_move(list, emittedMove);

    if (partialMove.captureCount >= MAX_CHAIN) {
        return;
    }

    for (index = 0; index < 4; ++index) {
        Position next = createPosition(current.row + rowSteps[index], current.col + colSteps[index]);
        Piece target;
        Move extendedMove;

        if (!position_is_reachable(next) || move_already_captures_square(&partialMove, next)) {
            continue;
        }

        target = getPiece(board, next);
        if (target.type != ANT || target.color == piece.color) {
            continue;
        }

        extendedMove = partialMove;
        addCapture(&extendedMove, next, target);
        append_anteater_capture_paths(board, next, piece, extendedMove, list);
    }
}

/* Sliding pieces all share the same scan pattern: stop at the first occupied
 * square and only keep the capture if that blocker belongs to the opponent. */
static void scan_sliding_direction(
    const Board *board,
    Position from,
    Piece piece,
    int rowStep,
    int colStep,
    MoveList *list
) {
    Position current;

    current = from;
    current.row += rowStep;
    current.col += colStep;

    while (position_is_reachable(current)) {
        Piece target = getPiece(board, current);
        Move move;

        if (is_friendly_piece(piece, target)) {
            break;
        }

        move = createMove(from, current, piece);
        if (is_capturable_enemy_piece(piece, target)) {
            addCapture(&move, current, target);
            add_candidate_move(list, move);
            break;
        }

        if (target.type != EMPTY_PIECE) {
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
static void generate_ant_moves(
    const GameState *state,
    Position from,
    Piece piece,
    MoveList *list
) {
    Position oneStep;
    Position twoStep;
    Position diagonal;
    int direction;
    int colOffset;

    direction = (piece.color == WHITE) ? -1 : 1;

    oneStep = createPosition(from.row + direction, from.col);
    if (position_is_reachable(oneStep) && getPiece(&state->board, oneStep).type == EMPTY_PIECE) {
        Move oneStepMove = createMove(from, oneStep, piece);

        if (is_promotion_square(piece, oneStep)) {
            append_promotion_moves(list, oneStepMove);
        } else {
            add_candidate_move(list, oneStepMove);
        }

        twoStep = createPosition(from.row + (2 * direction), from.col);
        if (is_starting_ant_row(piece, from)
            && position_is_reachable(twoStep)
            && getPiece(&state->board, twoStep).type == EMPTY_PIECE) {
            add_candidate_move(list, createMove(from, twoStep, piece));
        }
    }

    for (colOffset = -1; colOffset <= 1; colOffset += 2) {
        Piece target;
        Move move;

        diagonal = createPosition(from.row + direction, from.col + colOffset);
        if (!position_is_reachable(diagonal)) {
            continue;
        }

        target = getPiece(&state->board, diagonal);
        if (!is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = createMove(from, diagonal, piece);
        addCapture(&move, diagonal, target);
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
static void generate_anteater_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    static const int rowSteps[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int colSteps[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    int index;

    for (index = 0; index < 8; ++index) {
        Position to = createPosition(from.row + rowSteps[index], from.col + colSteps[index]);
        Piece target;
        Move move;

        if (!position_is_reachable(to)) {
            continue;
        }

        target = getPiece(board, to);
        if (is_friendly_piece(piece, target)) {
            continue;
        }

        move = createMove(from, to, piece);
        if (target.type == EMPTY_PIECE) {
            add_candidate_move(list, move);
            continue;
        }

        if (target.type == ANT && target.color != piece.color) {
            Move captureMove = move;

            addCapture(&captureMove, to, target);
            append_anteater_capture_paths(board, to, piece, captureMove, list);
        }
    }
}

/* Rooks generate along ranks and files until the first blocker. */
static void generate_rook_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    scan_sliding_direction(board, from, piece, -1, 0, list);
    scan_sliding_direction(board, from, piece, 1, 0, list);
    scan_sliding_direction(board, from, piece, 0, -1, list);
    scan_sliding_direction(board, from, piece, 0, 1, list);
}

/* Bishops generate along the four diagonals until the first blocker. */
static void generate_bishop_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    scan_sliding_direction(board, from, piece, -1, -1, list);
    scan_sliding_direction(board, from, piece, -1, 1, list);
    scan_sliding_direction(board, from, piece, 1, -1, list);
    scan_sliding_direction(board, from, piece, 1, 1, list);
}

/* Queens combine the rook and bishop movement patterns. */
static void generate_queen_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    generate_rook_moves(board, from, piece, list);
    generate_bishop_moves(board, from, piece, list);
}

/* Knights ignore blockers and only care about the landing square. */
static void generate_knight_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    static const int rowOffsets[] = {-2, -2, -1, -1, 1, 1, 2, 2};
    static const int colOffsets[] = {-1, 1, -2, 2, -2, 2, -1, 1};
    int index;

    for (index = 0; index < 8; ++index) {
        Position to = createPosition(from.row + rowOffsets[index], from.col + colOffsets[index]);
        Piece target;
        Move move;

        if (!position_is_reachable(to)) {
            continue;
        }

        target = getPiece(board, to);
        if (is_friendly_piece(piece, target)) {
            continue;
        }

        if (target.type != EMPTY_PIECE && !is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = createMove(from, to, piece);
        if (is_capturable_enemy_piece(piece, target)) {
            addCapture(&move, to, target);
        }
        add_candidate_move(list, move);
    }
}

/* Kings are single-step movers in any direction, with the same landing rules
 * as other ordinary capture pieces. */
static void generate_king_step_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    int rowOffset;
    int colOffset;

    for (rowOffset = -1; rowOffset <= 1; ++rowOffset) {
        for (colOffset = -1; colOffset <= 1; ++colOffset) {
            Position to;
            Piece target;
            Move move;

            if (rowOffset == 0 && colOffset == 0) {
                continue;
            }

            to = createPosition(from.row + rowOffset, from.col + colOffset);
            if (!position_is_reachable(to)) {
                continue;
            }

            target = getPiece(board, to);
            if (is_friendly_piece(piece, target)) {
                continue;
            }

            if (target.type != EMPTY_PIECE && !is_capturable_enemy_piece(piece, target)) {
                continue;
            }

            move = createMove(from, to, piece);
            if (is_capturable_enemy_piece(piece, target)) {
                addCapture(&move, to, target);
            }
            add_candidate_move(list, move);
        }
    }
}

/* Castling rights are reconstructed from history and the current board. */
static void append_castling_moves(
    const GameState *state,
    Position from,
    Piece piece,
    MoveList *list
) {
    Position kingStart;
    Position kingsideRookPos;
    Position queensideRookPos;
    Color enemyColor;
    Piece kingsideRook;
    Piece queensideRook;
    int row;

    if (state == NULL || piece.type != KING) {
        return;
    }

    row = (piece.color == WHITE) ? 7 : 0;
    kingStart = createPosition(row, 5);
    if (!positionEqual(from, kingStart) || history_touches_square(state, kingStart)) {
        return;
    }

    enemyColor = (piece.color == WHITE) ? BLACK : WHITE;
    if (isInCheck(state, piece.color) == 1) {
        return;
    }

    kingsideRookPos = createPosition(row, 9);
    kingsideRook = getPiece(&state->board, kingsideRookPos);
    if (kingsideRook.type == ROOK
        && kingsideRook.color == piece.color
        && history_touches_square(state, kingsideRookPos) == 0
        && getPiece(&state->board, createPosition(row, 6)).type == EMPTY_PIECE
        && getPiece(&state->board, createPosition(row, 7)).type == EMPTY_PIECE
        && getPiece(&state->board, createPosition(row, 8)).type == EMPTY_PIECE
        && square_is_attacked(&state->board, createPosition(row, 6), enemyColor) == 0
        && square_is_attacked(&state->board, createPosition(row, 7), enemyColor) == 0) {
        Move move = createMove(from, createPosition(row, 7), piece);

        setSpecialMove(&move, CASTLING_KINGSIDE);
        add_candidate_move(list, move);
    }

    queensideRookPos = createPosition(row, 0);
    queensideRook = getPiece(&state->board, queensideRookPos);
    if (queensideRook.type == ROOK
        && queensideRook.color == piece.color
        && history_touches_square(state, queensideRookPos) == 0
        && getPiece(&state->board, createPosition(row, 1)).type == EMPTY_PIECE
        && getPiece(&state->board, createPosition(row, 2)).type == EMPTY_PIECE
        && getPiece(&state->board, createPosition(row, 3)).type == EMPTY_PIECE
        && getPiece(&state->board, createPosition(row, 4)).type == EMPTY_PIECE
        && square_is_attacked(&state->board, createPosition(row, 4), enemyColor) == 0
        && square_is_attacked(&state->board, createPosition(row, 3), enemyColor) == 0) {
        Move move = createMove(from, createPosition(row, 3), piece);

        setSpecialMove(&move, CASTLING_QUEENSIDE);
        add_candidate_move(list, move);
    }
}

/* Dispatch piece-specific generation without exposing helper functions publicly. */
static void generate_pseudo_moves_for_position(
    const GameState *state,
    Position from,
    Piece piece,
    MoveList *list
) {
    switch (piece.type) {
        case ANT:
            generate_ant_moves(state, from, piece, list);
            break;
        case ANTEATER:
            generate_anteater_moves(&state->board, from, piece, list);
            break;
        case ROOK:
            generate_rook_moves(&state->board, from, piece, list);
            break;
        case BISHOP:
            generate_bishop_moves(&state->board, from, piece, list);
            break;
        case QUEEN:
            generate_queen_moves(&state->board, from, piece, list);
            break;
        case KNIGHT:
            generate_knight_moves(&state->board, from, piece, list);
            break;
        case KING:
            generate_king_step_moves(&state->board, from, piece, list);
            append_castling_moves(state, from, piece, list);
            break;
        case EMPTY_PIECE:
        default:
            break;
    }
}

/* Reject empty, off-turn, and out-of-bounds origins before generation begins. */
static int can_generate_from_position(const GameState *state, Position from, Piece *pieceOut) {
    Piece piece;

    if (state == NULL || !position_is_reachable(from)) {
        return 0;
    }

    piece = getPiece(&state->board, from);
    if (piece.type == EMPTY_PIECE || piece.color != state->currentTurn) {
        return 0;
    }

    if (pieceOut != NULL) {
        *pieceOut = piece;
    }

    return 1;
}

/* Filter pseudo-legal moves down to moves that leave the moving king safe. */
static int filter_legal_moves(const GameState *state, const MoveList *pseudo, MoveList *legal) {
    int index;

    if (state == NULL || pseudo == NULL || legal == NULL) {
        return 1;
    }

    initMoveList(legal);
    for (index = 0; index < pseudo->count; ++index) {
        GameState next;
        Color movingSide;

        next = *state;
        movingSide = state->currentTurn;

        /* Legal filtering only needs the resulting board position; clearing
         * history avoids MAX_MOVES capacity blocking analysis copies. */
        initMoveList(&next.moveHistory);
        next.moveCount = 0;
        next.gameOver = 0;
        next.result = RESULT_NONE;

        if (applyMove(&next, pseudo->moves[index]) != 0) {
            continue;
        }

        if (isInCheck(&next, movingSide) == 0) {
            addMove(legal, pseudo->moves[index]);
        }
    }

    return 0;
}

/* Generate every pseudo-legal candidate move for the side whose turn is stored
 * in state. */
int generateMoves(const GameState *state, MoveList *list) {
    int row;
    int col;

    if (state == NULL || list == NULL) {
        return 1;
    }

    initMoveList(list);
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece;
            Position from = createPosition(row, col);

            if (!can_generate_from_position(state, from, &piece)) {
                continue;
            }

            generate_pseudo_moves_for_position(state, from, piece, list);
        }
    }

    return 0;
}

/* The legal move API filters pseudo-legal candidates through self-check
 * detection. */
int generateLegalMoves(const GameState *state, MoveList *list) {
    MoveList pseudo;

    if (state == NULL || list == NULL) {
        return 1;
    }

    if (generateMoves(state, &pseudo) != 0) {
        return 1;
    }

    return filter_legal_moves(state, &pseudo, list);
}

/* Generate legal moves for one origin square if it belongs to the side to
 * move. */
int generateLegalMovesForPosition(const GameState *state, Position from, MoveList *list) {
    MoveList pseudo;
    Piece piece;

    if (state == NULL || list == NULL) {
        return 1;
    }

    initMoveList(list);
    if (!can_generate_from_position(state, from, &piece)) {
        return 0;
    }

    initMoveList(&pseudo);
    generate_pseudo_moves_for_position(state, from, piece, &pseudo);
    return filter_legal_moves(state, &pseudo, list);
}
