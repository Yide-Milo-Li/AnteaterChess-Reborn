#include "gameplay/execution.h"

#include <stddef.h>

/* Applies the desired move (from and to) */
int applyMove(GameState *state, Move move) {
    Piece currentPiece;
    Piece pieceToPlace;
    Piece rook;
    int captureIndex;

    /* Check for valid state and from and to */
    if (state == NULL || !isValidPosition(move.from) || !isValidPosition(move.to)) {
        return 1;
    }

    /* Gets currentPiece and perform checks */
    currentPiece = getPiece(&state->board, move.from);
    if (currentPiece.type == EMPTY_PIECE) {                // invalid from (empty space)
        return 1;
    }

    /* Checks if the piece on the board is different from 
     * the piece this move says it is moving */
    if (currentPiece.type != move.movedPiece.type || currentPiece.color != move.movedPiece.color) {
        return 1;
    }

    /* Checks if the color of the piece matches the color of the turn */
    if (currentPiece.color != state->currentTurn) {
        return 1;
    }

    /* Checks if moveHistory count is already at max capacity */
    if (state->moveHistory.count >= MAX_MOVES) {
        return 1;
    }

    /* Checks castling rook access before updating the board */
    rook = createPiece(EMPTY_PIECE, EMPTY_COLOR);
    if (move.specialType == CASTLING_KINGSIDE) {
        rook = getPiece(&state->board, createPosition(move.from.row, COLS - 1));
        if (rook.type != ROOK || rook.color != move.movedPiece.color) {
            return 1;
        }
    } else if (move.specialType == CASTLING_QUEENSIDE) {
        rook = getPiece(&state->board, createPosition(move.from.row, 0));
        if (rook.type != ROOK || rook.color != move.movedPiece.color) {
            return 1;
        }
    }

    /* Remove the piece from its starting square before applying captures or
     * special-case movement. */
    removePiece(&state->board, move.from);

    /* Each capture record stores the exact square and piece that should be
     * removed from the board for this move. */
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        removePiece(&state->board, move.captures[captureIndex].pos);
    }

    /* Set pieceToPlace to the movedPiece unless Promotion cases
     * where pieceToPlace is now dependent on chosen promotion piece */
    pieceToPlace = move.movedPiece;
    if (move.specialType == PROMOTION_QUEEN) {
        pieceToPlace = createPiece(QUEEN, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_ROOK) {
        pieceToPlace = createPiece(ROOK, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_BISHOP) {
        pieceToPlace = createPiece(BISHOP, move.movedPiece.color);
    } else if (move.specialType == PROMOTION_KNIGHT) {
        pieceToPlace = createPiece(KNIGHT, move.movedPiece.color);
    }

    /* Handle Castling */
    if (move.specialType == CASTLING_KINGSIDE) {            // Right Rook
        /* Remove the rook on the right and set it to the left of the king's destination */
        removePiece(&state->board, createPosition(move.from.row, COLS - 1));
        setPiece(&state->board, createPosition(move.from.row, move.to.col - 1), rook);
    } else if (move.specialType == CASTLING_QUEENSIDE) {    // Left Rook
        /* Remove the rook on the left and set it to the left of hte king's destionation*/
        removePiece(&state->board, createPosition(move.from.row, 0));
        setPiece(&state->board, createPosition(move.from.row, move.to.col + 1), rook);
    }

    /* Place the piece onto the destination (to) square*/
    setPiece(&state->board, move.to, pieceToPlace);

    /* Store the move after the board has been updated so undo can reconstruct
     * the exact pre-move position from history */
    if (addMoveToHistory(state, move) != 0) {
        return 1;
    }

    /* A completed move switch player turn */
    if (state->currentTurn == WHITE) {
        state->currentTurn = BLACK;
    } else {
        state->currentTurn = WHITE;
    }

    /* Until the hash module is connected to execution 
     * set it to the neutral placeholder value */
    state->hash = 0;
    return 0;
}

/* Undo the most recent move*/
int undoMove(GameState *state) {
    Move *move;
    int captureIndex;

    /* Check for empty state pointer or if this is the first turn */
    if (state == NULL || state->moveHistory.count <= 0) {
        return 1;
    }

    /* Get the most recent move */
    move = getMove(&state->moveHistory, state->moveHistory.count - 1);
    if (move == NULL) {
        return 1;
    }

    /* Remove the piece that currently occupies the destination square. For a
     * promotion this removes the promoted piece, not the original ant. */
    removePiece(&state->board, move->to);

    /* Put the original moving piece back where it started */
    setPiece(&state->board, move->from, move->movedPiece);

    /* Restore every captured piece to its original square. This also handles
     * anteater chain captures and en passant because the move already records
     * the exact capture positions. */
    for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
        setPiece(&state->board, move->captures[captureIndex].pos, move->captures[captureIndex].piece);
    }

    /* Undoing castling also moves the rook back to its starting square */
    if (move->specialType == CASTLING_KINGSIDE) {
        Piece rook = getPiece(&state->board, createPosition(move->from.row, move->to.col - 1));

        removePiece(&state->board, createPosition(move->from.row, move->to.col - 1));
        setPiece(&state->board, createPosition(move->from.row, COLS - 1), rook);
    } else if (move->specialType == CASTLING_QUEENSIDE) {
        Piece rook = getPiece(&state->board, createPosition(move->from.row, move->to.col + 1));

        removePiece(&state->board, createPosition(move->from.row, move->to.col + 1));
        setPiece(&state->board, createPosition(move->from.row, 0), rook);
    }

    /* Remove the most recent move from history */
    if (removeLastMoveFromHistory(state) != 0) {
        return 1;
    }

    /* The player who made the undone move becomes the current player again. */
    state->currentTurn = move->movedPiece.color;

    /* Undo backs the game out of any previously detected terminal result
     * because that result may no longer be true after the position changes */
    state->result = RESULT_NONE;
    state->gameOver = 0;
    state->hash = 0;
    return 0;
}
