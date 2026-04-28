#include "core/hash.h"
#include <stddef.h>
#include <stdint.h>

#include "core/board.h"
#include "core/piece.h"

#define HASH_PIECE_TYPES 7 // ant rook knight bishop queen king anteater

// random keys for Zobrist hashing
static uint64_t g_pieceKeys[2][HASH_PIECE_TYPES][ROWS][COLS]; // [color][type][row][col]
static uint64_t g_sideToMoveKey;                              // xor when black to move
static uint64_t g_castlingKeys[4];                            // 4 castling rights
static uint64_t g_enPassantKeys[COLS];                        // one key per file
static int g_zobristReady;                                    // init flag, set once

// splitmix64 PRNG, used to generate the random keys above
static uint64_t splitmix64(uint64_t *state) {
    uint64_t value;

    *state += 0x9e3779b97f4a7c15ULL;
    value = *state;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

// abs function for efficiency
static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

// PieceType -> index in g_pieceKeys, EMPTY = -1
static int piece_index(PieceType type) {
    switch (type) {
    case ANT:
        return 0;
    case ROOK:
        return 1;
    case KNIGHT:
        return 2;
    case BISHOP:
        return 3;
    case QUEEN:
        return 4;
    case KING:
        return 5;
    case ANTEATER:
        return 6;
    case EMPTY_PIECE:
    default:
        return -1;
    }
}

// look up the random key for a piece on a square (0 if empty)
static uint64_t piece_key(Piece piece, Position pos) {
    int index;

    if (piece.type == EMPTY_PIECE || piece.color == EMPTY_COLOR) {
        return 0;
    }

    index = piece_index(piece.type);
    if (index < 0) {
        return 0;
    }

    return g_pieceKeys[piece.color][index][pos.row][pos.col];
}

// has any past move involved this square (as from/to/capture)
// used to know if the king or rook has moved -> lose castling right
static int history_touches_square(const GameState *state, Position square) {
    int moveIndex;

    if (state == NULL) {
        return 0;
    }

    // scan every recorded move
    for (moveIndex = 0; moveIndex < state->moveHistory.count; ++moveIndex) {
        Move *move = getMove((MoveList *)&state->moveHistory, moveIndex);
        int captureIndex;

        if (move == NULL) {
            continue;
        }

        // square was the source or destination of a move
        if (positionEqual(move->from, square) || positionEqual(move->to, square)) {
            return 1;
        }

        // square held a captured piece (e.g. rook eaten by anteater)
        for (captureIndex = 0; captureIndex < move->captureCount; ++captureIndex) {
            if (positionEqual(move->captures[captureIndex].pos, square)) {
                return 1;
            }
        }
    }

    return 0;
}

// figure out current castling rights by checking start squares + history
// rebuild from scratch, used by deriveHashState (full hash)
static unsigned char castling_rights_for_state(const GameState *state) {
    static const Position whiteKingStart = {7, 5};
    static const Position whiteKingsideRookStart = {7, 9};
    static const Position whiteQueensideRookStart = {7, 0};
    static const Position blackKingStart = {0, 5};
    static const Position blackKingsideRookStart = {0, 9};
    static const Position blackQueensideRookStart = {0, 0};
    unsigned char rights;

    if (state == NULL) {
        return 0;
    }

    rights = 0;
    // white still has king on start square and never moved -> can still castle
    if (getPiece(&state->board, whiteKingStart).type == KING &&
        getPiece(&state->board, whiteKingStart).color == WHITE && history_touches_square(state, whiteKingStart) == 0) {
        // kingside rook untouched
        if (getPiece(&state->board, whiteKingsideRookStart).type == ROOK &&
            getPiece(&state->board, whiteKingsideRookStart).color == WHITE &&
            history_touches_square(state, whiteKingsideRookStart) == 0) {
            rights |= HASH_CASTLE_WHITE_KINGSIDE;
        }
        // queenside rook untouched
        if (getPiece(&state->board, whiteQueensideRookStart).type == ROOK &&
            getPiece(&state->board, whiteQueensideRookStart).color == WHITE &&
            history_touches_square(state, whiteQueensideRookStart) == 0) {
            rights |= HASH_CASTLE_WHITE_QUEENSIDE;
        }
    }

    // same check for black
    if (getPiece(&state->board, blackKingStart).type == KING &&
        getPiece(&state->board, blackKingStart).color == BLACK && history_touches_square(state, blackKingStart) == 0) {
        if (getPiece(&state->board, blackKingsideRookStart).type == ROOK &&
            getPiece(&state->board, blackKingsideRookStart).color == BLACK &&
            history_touches_square(state, blackKingsideRookStart) == 0) {
            rights |= HASH_CASTLE_BLACK_KINGSIDE;
        }
        if (getPiece(&state->board, blackQueensideRookStart).type == ROOK &&
            getPiece(&state->board, blackQueensideRookStart).color == BLACK &&
            history_touches_square(state, blackQueensideRookStart) == 0) {
            rights |= HASH_CASTLE_BLACK_QUEENSIDE;
        }
    }

    return rights;
}

// figure out if there is an en-passant target for current side
// only valid right after enemy ant push 2 step and our ant sit beside it
// return file (col), or HASH_NO_EN_PASSANT_FILE
static int en_passant_file_for_state(const GameState *state) {
    Move *lastMove;
    int file;
    int delta;
    int offset;
    int direction;
    Color enemyColor;
    Position target;

    if (state == NULL || state->moveHistory.count <= 0
        || (state->currentTurn != WHITE && state->currentTurn != BLACK)) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    // last move must be an ant moving in same column
    lastMove = getMove((MoveList *)&state->moveHistory, state->moveHistory.count - 1);
    if (lastMove == NULL || lastMove->movedPiece.type != ANT || lastMove->from.col != lastMove->to.col) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    // must be a 2-step push
    delta = absolute_value(lastMove->to.row - lastMove->from.row);
    if (delta != 2) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    // current turn must be the side that can capture
    file = lastMove->to.col;
    enemyColor = (lastMove->movedPiece.color == WHITE) ? BLACK : WHITE;
    if (state->currentTurn != enemyColor) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    // the square behind the pushed ant must be empty (capture lands here)
    direction = (state->currentTurn == WHITE) ? -1 : 1;
    target = createPosition(lastMove->to.row + direction, file);
    if (!isValidPosition(target) || getPiece(&state->board, target).type != EMPTY_PIECE) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    // need our ant on left or right to actually do en-passant
    for (offset = -1; offset <= 1; offset += 2) {
        Position adjacent = createPosition(lastMove->to.row, file + offset);
        Piece adjacentPiece;

        if (!isValidPosition(adjacent)) {
            continue;
        }

        adjacentPiece = getPiece(&state->board, adjacent);
        if (adjacentPiece.type == ANT && adjacentPiece.color == enemyColor) {
            return file;
        }
    }

    return HASH_NO_EN_PASSANT_FILE;
}

// SpecialMove -> resulting PieceType after promotion, or EMPTY if not promotion
static int promotion_piece_type(SpecialMove type) {
    switch (type) {
    case PROMOTION_QUEEN:
        return QUEEN;
    case PROMOTION_ROOK:
        return ROOK;
    case PROMOTION_BISHOP:
        return BISHOP;
    case PROMOTION_KNIGHT:
        return KNIGHT;
    case NO_SPECIAL_MOVE:
    case CASTLING_KINGSIDE:
    case CASTLING_QUEENSIDE:
    case EN_PASSANT:
    case ANTEATER_CAPTURE:
    default:
        return EMPTY_PIECE;
    }
}

// what piece will sit on `to` after this move (promotion change type)
static Piece piece_after_move(Move move) {
    Piece placedPiece;
    PieceType promotedType;

    placedPiece = move.movedPiece;
    promotedType = promotion_piece_type(move.specialType);
    if (promotedType != EMPTY_PIECE) {
        placedPiece.type = promotedType;
    }

    return placedPiece;
}

// fill the random key tables, only run once (fixed seed for reproducible hash)
void initZobrist(void) {
    uint64_t seed;
    int color;
    int type;
    int row;
    int col;
    int index;

    if (g_zobristReady) {
        return;
    }

    // generate a unique key for every (color, piece type, square)
    seed = 0x1f2e3d4c5b6a7988ULL;
    for (color = 0; color < 2; ++color) {
        for (type = 0; type < HASH_PIECE_TYPES; ++type) {
            for (row = 0; row < ROWS; ++row) {
                for (col = 0; col < COLS; ++col) {
                    g_pieceKeys[color][type][row][col] = splitmix64(&seed);
                }
            }
        }
    }

    // side to move + 4 castling rights + per-file en passant
    g_sideToMoveKey = splitmix64(&seed);
    for (index = 0; index < 4; ++index) {
        g_castlingKeys[index] = splitmix64(&seed);
    }
    for (index = 0; index < COLS; ++index) {
        g_enPassantKeys[index] = splitmix64(&seed);
    }

    g_zobristReady = 1;
}

// build the full HashState from scratch, by xoring keys for every piece, side, castling and en-passant
// used at game start or when no previous hash is available
int deriveHashState(const GameState *state, HashState *out) {
    uint64_t value;
    int row;
    int col;

    if (state == NULL || out == NULL) {
        return 1;
    }

    initZobrist();
    value = 0;
    // xor a key for every piece on the board
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(&state->board, pos);

            value ^= piece_key(piece, pos);
        }
    }

    // also mix in turn, castling rights, en-passant file
    out->castlingRights = castling_rights_for_state(state);
    out->enPassantFile = en_passant_file_for_state(state);
    if (state->currentTurn == BLACK) {
        value ^= g_sideToMoveKey;
    }
    if ((out->castlingRights & HASH_CASTLE_WHITE_KINGSIDE) != 0) {
        value ^= g_castlingKeys[0];
    }
    if ((out->castlingRights & HASH_CASTLE_WHITE_QUEENSIDE) != 0) {
        value ^= g_castlingKeys[1];
    }
    if ((out->castlingRights & HASH_CASTLE_BLACK_KINGSIDE) != 0) {
        value ^= g_castlingKeys[2];
    }
    if ((out->castlingRights & HASH_CASTLE_BLACK_QUEENSIDE) != 0) {
        value ^= g_castlingKeys[3];
    }
    if (out->enPassantFile != HASH_NO_EN_PASSANT_FILE) {
        value ^= g_enPassantKeys[out->enPassantFile];
    }

    out->value = value;
    return 0;
}

// shortcut: just return the 64-bit hash value
uint64_t computeHash(const GameState *state) {
    HashState hashState;

    if (deriveHashState(state, &hashState) != 0) {
        return 0;
    }

    return hashState.value;
}

// quick non-zobrist mix of move bytes into hash, used as a cheap distinguishing key
uint64_t updateHashMove(uint64_t hash, Move move) {
    uint64_t mix;

    // pack move fields into one uint64
    mix = (uint64_t)(move.from.row & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.from.col & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.to.row & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.to.col & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.specialType & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.movedPiece.type & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.movedPiece.color & 0xff);
    // xorshift to scatter the bits
    mix ^= mix << 17;
    mix ^= mix >> 13;
    mix ^= mix << 7;
    return hash ^ mix;
}

// incremental update: take the hash before the move and produce the hash after
// much faster than rebuilding from board, only xor in/out the changes
int advanceHashState(const GameState *after, Move move, const HashState *previous, HashState *next) {
    Position rookFrom;
    Position rookTo;
    Piece movedToPiece;
    unsigned char newRights;
    uint64_t value;
    int captureIndex;

    if (after == NULL || previous == NULL || next == NULL) {
        return 1;
    }

    initZobrist();
    value = previous->value;

    // remove old en-passant key (it only valid for one ply)
    if (previous->enPassantFile != HASH_NO_EN_PASSANT_FILE) {
        value ^= g_enPassantKeys[previous->enPassantFile];
    }

    // flip side to move
    value ^= g_sideToMoveKey;
    // pull moved piece off `from`
    value ^= piece_key(move.movedPiece, move.from);
    // remove every captured piece from its square
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        value ^= piece_key(move.captures[captureIndex].piece, move.captures[captureIndex].pos);
    }

    // place piece on `to` (might be promoted into a queen/rook/bishop/knight)
    movedToPiece = piece_after_move(move);
    value ^= piece_key(movedToPiece, move.to);

    // castling: also move the rook in/out of its squares
    if (move.specialType == CASTLING_KINGSIDE) {
        rookFrom = createPosition(move.from.row, COLS - 1);
        rookTo = createPosition(move.from.row, move.to.col - 1);
        value ^= piece_key(createPiece(ROOK, move.movedPiece.color), rookFrom);
        value ^= piece_key(createPiece(ROOK, move.movedPiece.color), rookTo);
    } else if (move.specialType == CASTLING_QUEENSIDE) {
        rookFrom = createPosition(move.from.row, 0);
        rookTo = createPosition(move.from.row, move.to.col + 1);
        value ^= piece_key(createPiece(ROOK, move.movedPiece.color), rookFrom);
        value ^= piece_key(createPiece(ROOK, move.movedPiece.color), rookTo);
    }

    // update castling rights based on what moved
    newRights = previous->castlingRights;
    // king move kills both side castle right
    if (move.movedPiece.type == KING) {
        if (move.movedPiece.color == WHITE) {
            newRights &= (unsigned char)~(HASH_CASTLE_WHITE_KINGSIDE | HASH_CASTLE_WHITE_QUEENSIDE);
        } else {
            newRights &= (unsigned char)~(HASH_CASTLE_BLACK_KINGSIDE | HASH_CASTLE_BLACK_QUEENSIDE);
        }
        // rook move kills only its own corner
    } else if (move.movedPiece.type == ROOK) {
        if (move.movedPiece.color == WHITE) {
            if (move.from.row == 7 && move.from.col == 9) {
                newRights &= (unsigned char)~HASH_CASTLE_WHITE_KINGSIDE;
            } else if (move.from.row == 7 && move.from.col == 0) {
                newRights &= (unsigned char)~HASH_CASTLE_WHITE_QUEENSIDE;
            }
        } else {
            if (move.from.row == 0 && move.from.col == 9) {
                newRights &= (unsigned char)~HASH_CASTLE_BLACK_KINGSIDE;
            } else if (move.from.row == 0 && move.from.col == 0) {
                newRights &= (unsigned char)~HASH_CASTLE_BLACK_QUEENSIDE;
            }
        }
    }

    // a captured rook on its start corner also kills castling right
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        CaptureRecord capture = move.captures[captureIndex];

        if (capture.piece.type != ROOK) {
            continue;
        }

        if (capture.piece.color == WHITE) {
            if (capture.pos.row == 7 && capture.pos.col == 9) {
                newRights &= (unsigned char)~HASH_CASTLE_WHITE_KINGSIDE;
            } else if (capture.pos.row == 7 && capture.pos.col == 0) {
                newRights &= (unsigned char)~HASH_CASTLE_WHITE_QUEENSIDE;
            }
        } else {
            if (capture.pos.row == 0 && capture.pos.col == 9) {
                newRights &= (unsigned char)~HASH_CASTLE_BLACK_KINGSIDE;
            } else if (capture.pos.row == 0 && capture.pos.col == 0) {
                newRights &= (unsigned char)~HASH_CASTLE_BLACK_QUEENSIDE;
            }
        }
    }

    // xor the castling keys for every right that flipped
    if (((previous->castlingRights ^ newRights) & HASH_CASTLE_WHITE_KINGSIDE) != 0) {
        value ^= g_castlingKeys[0];
    }
    if (((previous->castlingRights ^ newRights) & HASH_CASTLE_WHITE_QUEENSIDE) != 0) {
        value ^= g_castlingKeys[1];
    }
    if (((previous->castlingRights ^ newRights) & HASH_CASTLE_BLACK_KINGSIDE) != 0) {
        value ^= g_castlingKeys[2];
    }
    if (((previous->castlingRights ^ newRights) & HASH_CASTLE_BLACK_QUEENSIDE) != 0) {
        value ^= g_castlingKeys[3];
    }

    next->castlingRights = newRights;
    next->enPassantFile = HASH_NO_EN_PASSANT_FILE;
    // detect ant double push to set new en-passant file (same logic as derive)
    if (move.movedPiece.type == ANT && move.from.col == move.to.col &&
        absolute_value(move.to.row - move.from.row) == 2) {
        int offset;
        int direction;
        Color enemyColor;
        Position target;

        // turn must now belong to the enemy
        enemyColor = (move.movedPiece.color == WHITE) ? BLACK : WHITE;
        if (after->currentTurn != enemyColor) {
            next->value = value;
            return 0;
        }

        // capture target square must be empty
        direction = (after->currentTurn == WHITE) ? -1 : 1;
        target = createPosition(move.to.row + direction, move.to.col);
        if (!isValidPosition(target) || getPiece(&after->board, target).type != EMPTY_PIECE) {
            next->value = value;
            return 0;
        }

        // need an enemy ant beside the pushed ant to actually capture
        for (offset = -1; offset <= 1; offset += 2) {
            Position adjacent = createPosition(move.to.row, move.to.col + offset);
            Piece adjacentPiece;

            if (!isValidPosition(adjacent)) {
                continue;
            }

            adjacentPiece = getPiece(&after->board, adjacent);
            if (adjacentPiece.type == ANT && adjacentPiece.color == enemyColor) {
                next->enPassantFile = move.to.col;
                break;
            }
        }
    }

    // mix in the new en-passant file key, if any
    if (next->enPassantFile != HASH_NO_EN_PASSANT_FILE) {
        value ^= g_enPassantKeys[next->enPassantFile];
    }

    next->value = value;
    return 0;
}
