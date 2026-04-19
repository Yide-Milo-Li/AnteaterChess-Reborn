#include "gameplay/movegen.h"

#include <stddef.h>

#include "core/board.h"
#include "core/move.h"
#include "core/piece.h"
#include "core/position.h"

/*
 * Alignment assumptions for future extensions:
 * - This file generates move candidates for gameplay; it does not mutate GameState.
 * - Public behavior must stay aligned with include/gameplay/movegen.h.
 * - Legal move generation excludes direct king captures; check detection belongs elsewhere.
 * - Piece-specific helpers may grow, but shared boundary checks should stay centralized.
 */

/* Centralize coordinate math so every generator rejects out-of-bounds targets
 * before touching board accessors. */
static int position_is_reachable(Position pos) {
    return isValidPosition(pos);
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
 * diagonals. The double-step is legal only from the starting row and only if
 * both forward squares are empty. */
static void generate_ant_moves(
    const Board *board,
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
    if (position_is_reachable(oneStep) && getPiece(board, oneStep).type == EMPTY_PIECE) {
        add_candidate_move(list, createMove(from, oneStep, piece));

        twoStep = createPosition(from.row + (2 * direction), from.col);
        if (is_starting_ant_row(piece, from)
            && position_is_reachable(twoStep)
            && getPiece(board, twoStep).type == EMPTY_PIECE) {
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

        target = getPiece(board, diagonal);
        if (!is_capturable_enemy_piece(piece, target)) {
            continue;
        }

        move = createMove(from, diagonal, piece);
        addCapture(&move, diagonal, target);
        add_candidate_move(list, move);
    }
}

/* Anteaters can move one square in any direction, but they only capture ants.
 * The special chain capture is a straight orthogonal run through enemy ants,
 * and each longer prefix of that run is a valid destination. */
static void generate_anteater_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    static const int rowSteps[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    static const int colSteps[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int chainRowSteps[] = {-1, 1, 0, 0};
    static const int chainColSteps[] = {0, 0, -1, 1};
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
            addCapture(&move, to, target);
            setSpecialMove(&move, ANTEATER_CAPTURE);
            add_candidate_move(list, move);
        }
    }

    for (index = 0; index < 4; ++index) {
        Move chainMove;
        Position current;
        int captureCount;

        current = createPosition(from.row + chainRowSteps[index], from.col + chainColSteps[index]);
        chainMove = createMove(from, current, piece);
        captureCount = 0;

        while (position_is_reachable(current)) {
            Piece target = getPiece(board, current);

            if (target.type != ANT || target.color == piece.color) {
                break;
            }

            addPathStep(&chainMove, current);
            addCapture(&chainMove, current, target);
            setSpecialMove(&chainMove, ANTEATER_CAPTURE);
            ++captureCount;

            /* Distance-one orthogonal captures are already covered by the
             * standard one-step rule, so only longer chains are emitted here. */
            if (captureCount >= 2) {
                chainMove.to = current;
                add_candidate_move(list, chainMove);
            }

            current.row += chainRowSteps[index];
            current.col += chainColSteps[index];
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
static void generate_king_moves(
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

/* Dispatch piece-specific generation without exposing helper functions publicly. */
static void generate_piece_moves(
    const Board *board,
    Position from,
    Piece piece,
    MoveList *list
) {
    switch (piece.type) {
        case ANT:
            generate_ant_moves(board, from, piece, list);
            break;
        case ANTEATER:
            generate_anteater_moves(board, from, piece, list);
            break;
        case ROOK:
            generate_rook_moves(board, from, piece, list);
            break;
        case BISHOP:
            generate_bishop_moves(board, from, piece, list);
            break;
        case QUEEN:
            generate_queen_moves(board, from, piece, list);
            break;
        case KNIGHT:
            generate_knight_moves(board, from, piece, list);
            break;
        case KING:
            generate_king_moves(board, from, piece, list);
            break;
        case EMPTY_PIECE:
        default:
            break;
    }
}

/* Reject empty, off-turn, and out-of-bounds origins before generation begins. */
static int can_generate_from_position(const GameState *state, Position from, Piece *pieceOut) {
    Piece piece;

    if (!position_is_reachable(from)) {
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

/* Generate every candidate move for the side whose turn is stored in state. */
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

            generate_piece_moves(&state->board, from, piece, list);
        }
    }

    return 0;
}

/* Reuse the same generator until stricter legality semantics are introduced. */
int generateLegalMoves(const GameState *state, MoveList *list) {
    return generateMoves(state, list);
}

/* Generate moves for one origin square if it belongs to the side to move. */
int generateLegalMovesForPosition(const GameState *state, Position from, MoveList *list) {
    Piece piece;

    if (state == NULL || list == NULL) {
        return 1;
    }

    initMoveList(list);
    if (!can_generate_from_position(state, from, &piece)) {
        return 0;
    }

    generate_piece_moves(&state->board, from, piece, list);
    return 0;
}
