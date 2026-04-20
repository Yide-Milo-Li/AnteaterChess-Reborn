#include "gameplay/endgame.h"

#include <stddef.h>

#include "gameplay/execution.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"

/*
 * Alignment assumptions for future extensions:
 * - This file owns endgame evaluation, including direct attack detection for kings.
 * - Legal move generation excludes direct king captures, so check detection must not rely on them.
 * - Trial states used for mate/stalemate analysis are analysis-only copies with reset history bookkeeping.
 * - Public behavior must stay aligned with include/gameplay/endgame.h.
 */

/* Return the non-negative magnitude of an integer delta. */
static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

/* Sliding attack checks must stop at the first blocker between attacker and
 * target because legal move generation now excludes direct king captures. */
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

/* Anteaters do not attack kings under this ruleset, so they never contribute
 * to check detection. */
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

/* Endgame analysis works on copied states, so reset local history bookkeeping
 * before trial moves to avoid MAX_MOVES capacity interfering with evaluation. */
static void initialize_trial_state(GameState *trialState, const GameState *state, Color color) {
    *trialState = *state;
    trialState->currentTurn = color;
    initMoveList(&trialState->moveHistory);
    trialState->moveCount = 0;
    trialState->gameOver = 0;
    trialState->result = RESULT_NONE;
}

/* Check whether the specified king is currently under attack */
int isInCheck(const GameState *state, Color color) {
    Position kingPosition;
    Color attackingColor;
    int kingFound;
    int row;
    int col;

    if (state == NULL) {
        return 0;
    }

    if (color != WHITE && color != BLACK) {
        return 0;
    }

    kingPosition = createPosition(-1, -1);
    kingFound = 0;

    /* First find the king that belongs to the color we are evaluating. */
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));

            if (piece.type == KING && piece.color == color) {
                kingPosition = createPosition(row, col);
                kingFound = 1;
            }
        }
    }

    if (kingFound == 0) {
        return 0;
    }

    if (color == WHITE) {
        attackingColor = BLACK;
    } else {
        attackingColor = WHITE;
    }

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(&state->board, from);

            if (piece.type == EMPTY_PIECE || piece.color != attackingColor) {
                continue;
            }

            if (piece_attacks_square(&state->board, from, piece, kingPosition) == 1) {
                return 1;
            }
        }
    }

    return 0;
}

/* A player is checkmated when they are in check and none of their available
 * moves can produce a position where their king is safe. */
int isCheckmate(const GameState *state, Color color) {
    GameState moveGenerationState;
    MoveList candidates;
    int index;

    if (state == NULL) {
        return 0;
    }

    if (color != WHITE && color != BLACK) {
        return 0;
    }

    if (isInCheck(state, color) == 0) {
        return 0;
    }

    /* Generate moves as if the checked side is the side to play right now. */
    moveGenerationState = *state;
    moveGenerationState.currentTurn = color;

    if (generateLegalMoves(&moveGenerationState, &candidates) != 0) {
        return 0;
    }

    /* Try every candidate move on a copied state. If even one move gets the
     * player out of check, then the position is not checkmate. */
    for (index = 0; index < getMoveCount(&candidates); ++index) {
        Move *candidate = getMove(&candidates, index);
        GameState trialState;

        if (candidate == NULL) {
            continue;
        }

        /* Work on a copy so the original game state is never modified just to
         * answer the endgame question. */
        initialize_trial_state(&trialState, state, color);

        /* If a generated move somehow fails to apply, skip it and keep looking
         * at the rest of the move list. */
        if (applyMove(&trialState, *candidate) != 0) {
            continue;
        }

        /* applyMove switches the turn, but we still want to ask whether the
         * same player who started in check is still in check after the move. */
        if (isInCheck(&trialState, color) == 0) {
            return 0;
        }
    }

    /* Every legal candidate still left the king attacked, so the checked side
     * has no escape. */
    return 1;
}

/* A player is stalemated when they are not in check, but every available move
 * still leaves them with no legal position to continue from. */
int isStalemate(const GameState *state, Color color) {
    GameState moveGenerationState;
    MoveList candidates;
    int index;

    if (state == NULL) {
        return 0;
    }

    if (color != WHITE && color != BLACK) {
        return 0;
    }

    if (isInCheck(state, color) == 1) {
        return 0;
    }

    /* Generate moves as if the side we are testing is the side that must play
     * right now */
    moveGenerationState = *state;
    moveGenerationState.currentTurn = color;

    if (generateLegalMoves(&moveGenerationState, &candidates) != 0) {
        return 0;
    }

    /* Try every candidate move on a copied state. If even one move leads to a
     * legal position where the player is still not in check, then the player
     * is not stalemated. */
    for (index = 0; index < getMoveCount(&candidates); ++index) {
        Move *candidate = getMove(&candidates, index);
        GameState trialState;

        if (candidate == NULL) {
            continue;
        }

        /* Work on a copy so stalemate detection never mutates the real game. */
        initialize_trial_state(&trialState, state, color);

        /* If a generated move cannot be applied, skip it and keep checking the
         * rest of the move list. */
        if (applyMove(&trialState, *candidate) != 0) {
            continue;
        }

        /* The player is only stalemated if every candidate move still fails to
         * produce a valid continuing position. */
        if (isInCheck(&trialState, color) == 0) {
            return 0;
        }
    }

    /* The player was not in check to begin with, but no candidate move gave
     * them a legal way to continue */
    return 1;
}

/* Detect simple "not enough force left to finish the game" positions. This is
 * intentionally conservative: obvious mating material such as ants, rooks, and
 * queens always counts as sufficient, while lone minor pieces and same-color
 * bishops are treated as insufficient. Anteaters are also treated as
 * insufficient because they cannot capture kings under this ruleset. */
int isInsufficientMaterial(const GameState *state) {
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

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));
            int squareColor;

            /* Kings do not count toward mating material, and empty squares are
             * ignored entirely. */
            if (piece.type == EMPTY_PIECE || piece.type == KING) {
                continue;
            }

            ++totalNonKingPieces;

            /* Count each remaining piece type so the draw rules can make a
             * simple decision after the full board scan is done. */
            if (piece.type == ANT) {
                ++totalAnts;
            } else if (piece.type == ROOK) {
                ++totalRooks;
            } else if (piece.type == QUEEN) {
                ++totalQueens;
            } else if (piece.type == BISHOP) {
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
            } else if (piece.type == KNIGHT) {
                ++totalKnights;
            } else if (piece.type == ANTEATER) {
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

/* Update the GameState result field when a terminal position is found.
 * The currentTurn field identifies the side that must move next, so that is
 * the side we test for checkmate or stalemate. */
int detectGameResult(GameState *state) {
    Color playerToMove;

    if (state == NULL) {
        return 0;
    }

    if (state->currentTurn != WHITE && state->currentTurn != BLACK) {
        setGameResult(state, RESULT_NONE);
        return 0;
    }

    /* Endgame checks are always evaluated from the side that must move now. */
    playerToMove = state->currentTurn;

    /* If the side to move is checkmated, the other side has won. */
    if (isCheckmate(state, playerToMove) == 1) {
        if (playerToMove == WHITE) {
            setGameTermination(state, RESULT_BLACK_WIN, TERMINATION_CHECKMATE);
        } else {
            setGameTermination(state, RESULT_WHITE_WIN, TERMINATION_CHECKMATE);
        }
        return 1;
    }

    /* If the side to move has no legal continuation without being in check,
     * record a draw. */
    if (isStalemate(state, playerToMove) == 1) {
        setGameTermination(state, RESULT_DRAW, TERMINATION_STALEMATE);
        return 1;
    }

    /* If there is not enough material left to force a win, record a draw. */
    if (isInsufficientMaterial(state) == 1) {
        setGameTermination(state, RESULT_DRAW, TERMINATION_INSUFFICIENT_MATERIAL);
        return 1;
    }

    /* Otherwise the game should continue normally. */
    setGameResult(state, RESULT_NONE);
    return 0;
}
