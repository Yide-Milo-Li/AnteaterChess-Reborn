#include "core/move.h"
#include "core/board.h"

/* * Helper: Get positive number 
 */
int get_abs(int x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

/* * Helper: Check if there are pieces blocking the way
 */
int is_path_clear(const Board *board, Position from, Position to) {
    int rStep = 0;
    int cStep = 0;
    
    // Find row direction
    if (to.row > from.row) rStep = 1;
    else if (to.row < from.row) rStep = -1;
    
    // Find col direction
    if (to.col > from.col) cStep = 1;
    else if (to.col < from.col) cStep = -1;
    
    Position current = from;
    current.row += rStep;
    current.col += cStep;
    
    // Check all squares between from and to
    while (current.row != to.row || current.col != to.col) {
        if (getPiece(board, current).type != EMPTY_PIECE) {
            return 0; // Blocked!
        }
        current.row += rStep;
        current.col += cStep;
    }
    return 1; // Clear!
}

/* --------------------------------------------------------- */

/*
 * Rule for Anteater: Move 1 step OR eat a line of Ants
 */
int validate_anteater(const Board *board, Move *move) {
    int rDist = get_abs(move->to.row - move->from.row);
    int cDist = get_abs(move->to.col - move->from.col);
    Piece shooter = move->movedPiece;
    Piece target = getPiece(board, move->to);

    // Case 1: Move 1 step
    if (rDist <= 1 && cDist <= 1) {
        if (target.type == EMPTY_PIECE) {
            return 1;
        }
        // Can only eat enemy Ant
        if (target.type == ANT && shooter.color != target.color) {
            addCapture(move, move->to, target);
            setSpecialMove(move, ANTEATER_CAPTURE);
            return 1;
        }
        return 0;
    }

    // Case 2: Chain eating in a straight line
    if (move->from.row == move->to.row || move->from.col == move->to.col) {
        int rStep = 0;
        int cStep = 0;
        
        if (move->to.row > move->from.row) rStep = 1;
        else if (move->to.row < move->from.row) rStep = -1;
        
        if (move->to.col > move->from.col) cStep = 1;
        else if (move->to.col < move->from.col) cStep = -1;

        Position current = move->from;
        current.row += rStep;
        current.col += cStep;

        while (1) {
            Piece p = getPiece(board, current);
            
            // Must be enemy Ant
            if (p.type != ANT || p.color == shooter.color) {
                return 0; 
            }

            // Save the eaten ant
            addPathStep(move, current);
            addCapture(move, current, p);

            // Stop if we reached the target
            if (current.row == move->to.row && current.col == move->to.col) {
                break;
            }
            
            current.row += rStep;
            current.col += cStep;
        }
        setSpecialMove(move, ANTEATER_CAPTURE);
        return 1;
    }
    return 0;
}

/*
 * Rule for Ant: Move forward or eat diagonal
 */
int validate_ant(const Board *board, Move *move) {
    int dir;
    if (move->movedPiece.color == WHITE) {
        dir = -1; // White moves up
    } else {
        dir = 1;  // Black moves down
    }

    int rDist = move->to.row - move->from.row;
    int cDist = get_abs(move->to.col - move->from.col);
    Piece target = getPiece(board, move->to);

    // Move straight
    if (cDist == 0 && target.type == EMPTY_PIECE) {
        // 1 step
        if (rDist == dir) {
            return 1;
        }
        // 2 steps (only at start)
        int whiteStart = (move->movedPiece.color == WHITE && move->from.row == 6);
        int blackStart = (move->movedPiece.color == BLACK && move->from.row == 1);
        
        if ((whiteStart || blackStart) && rDist == 2 * dir) {
            Position mid;
            mid.row = move->from.row + dir;
            mid.col = move->from.col;
            if (getPiece(board, mid).type == EMPTY_PIECE) {
                return 1;
            }
        }
    }
    // Eat diagonal
    else if (cDist == 1 && rDist == dir) {
        if (target.type != EMPTY_PIECE && target.color != move->movedPiece.color) {
            addCapture(move, move->to, target);
            return 1;
        }
    }
    return 0;
}

/*
 * Rule for Rook: Straight lines
 */
int validate_rook(const Board *board, Move *move) {
    if (move->from.row != move->to.row && move->from.col != move->to.col) {
        return 0; // Not a straight line
    }
    if (is_path_clear(board, move->from, move->to) == 0) {
        return 0;
    }
    
    Piece target = getPiece(board, move->to);
    if (target.type != EMPTY_PIECE) {
        addCapture(move, move->to, target);
    }
    return 1;
}

/*
 * Rule for Bishop: Diagonals
 */
int validate_bishop(const Board *board, Move *move) {
    int rDist = get_abs(move->from.row - move->to.row);
    int cDist = get_abs(move->from.col - move->to.col);
    
    if (rDist != cDist) {
        return 0; // Not a diagonal
    }
    if (is_path_clear(board, move->from, move->to) == 0) {
        return 0;
    }
    
    Piece target = getPiece(board, move->to);
    if (target.type != EMPTY_PIECE) {
        addCapture(move, move->to, target);
    }
    return 1;
}

/*
 * Rule for Queen: Straight + Diagonal
 */
int validate_queen(const Board *board, Move *move) {
    int rDist = get_abs(move->from.row - move->to.row);
    int cDist = get_abs(move->from.col - move->to.col);
    
    if (move->from.row == move->to.row || move->from.col == move->to.col) {
        return validate_rook(board, move);
    } 
    if (rDist == cDist) {
        return validate_bishop(board, move);
    }
    return 0;
}

/*
 * Rule for Knight: L-shape jump
 */
int validate_knight(const Board *board, Move *move) {
    int rDist = get_abs(move->from.row - move->to.row);
    int cDist = get_abs(move->from.col - move->to.col);
    
    if ((rDist == 2 && cDist == 1) || (rDist == 1 && cDist == 2)) {
        Piece target = getPiece(board, move->to);
        if (target.type != EMPTY_PIECE) {
            addCapture(move, move->to, target);
        }
        return 1;
    }
    return 0;
}

/*
 * Rule for King: 1 step anywhere
 */
int validate_king(const Board *board, Move *move) {
    int rDist = get_abs(move->from.row - move->to.row);
    int cDist = get_abs(move->from.col - move->to.col);
    
    if (rDist <= 1 && cDist <= 1) {
        Piece target = getPiece(board, move->to);
        if (target.type != EMPTY_PIECE) {
            addCapture(move, move->to, target);
        }
        return 1;
    }
    return 0;
}

/* --- Main Check Function --- */

/*
 * Check if the move is valid for the piece
 */
int is_valid_piece_move(const Board *board, Move *move) {
    Piece target = getPiece(board, move->to);

    // Can not eat your own piece
    if (target.type != EMPTY_PIECE && move->movedPiece.color == target.color) {
        return 0;
    }

    if (move->movedPiece.type == ANT) return validate_ant(board, move);
    if (move->movedPiece.type == ANTEATER) return validate_anteater(board, move);
    if (move->movedPiece.type == ROOK) return validate_rook(board, move);
    if (move->movedPiece.type == BISHOP) return validate_bishop(board, move);
    if (move->movedPiece.type == QUEEN) return validate_queen(board, move);
    if (move->movedPiece.type == KNIGHT) return validate_knight(board, move);
    if (move->movedPiece.type == KING) return validate_king(board, move);
    
    return 0;
}