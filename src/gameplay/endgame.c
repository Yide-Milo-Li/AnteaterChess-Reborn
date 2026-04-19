#include "gameplay/endgame.h"

#include <stddef.h>

#include "gameplay/execution.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"

/* Check whether the specified king is currently under attack */
int isInCheck(const GameState *state, Color color) {
    GameState attackState;
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

    /* Reuse the existing move validator by pretending it is the opponent's
     * turn and asking whether any opposing piece can legally move to the king. */
    attackState = *state;
    attackState.currentTurn = attackingColor;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position from = createPosition(row, col);
            Piece piece = getPiece(&state->board, from);
            Move attackMove;

            if (piece.type == EMPTY_PIECE || piece.color != attackingColor) {
                continue;
            }

            attackMove = createMove(from, kingPosition, piece);
            if (validateMove(&attackState, attackMove) == 1) {
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
        trialState = *state;
        trialState.currentTurn = color;

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
        trialState = *state;
        trialState.currentTurn = color;

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
            setGameResult(state, RESULT_BLACK_WIN);
        } else {
            setGameResult(state, RESULT_WHITE_WIN);
        }
        return 1;
    }

    /* If the side to move has no legal continuation without being in check,
     * or if there is not enough material left to force a win, record a draw. */
    if (isStalemate(state, playerToMove) == 1 || isInsufficientMaterial(state) == 1) {
        setGameResult(state, RESULT_DRAW);
        return 1;
    }

    /* Otherwise the game should continue normally. */
    setGameResult(state, RESULT_NONE);
    return 0;
}
