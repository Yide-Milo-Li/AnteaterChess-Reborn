#include "core/hash.h"
#include <stddef.h>
#include <stdint.h>

#include "core/board.h"
#include "core/piece.h"

#define HASH_PIECE_TYPES 7

static uint64_t g_pieceKeys[2][HASH_PIECE_TYPES][ROWS][COLS];
static uint64_t g_sideToMoveKey;
static uint64_t g_castlingKeys[4];
static uint64_t g_enPassantKeys[COLS];
static int g_zobristReady;

static uint64_t splitmix64(uint64_t *state) {
    uint64_t value;

    *state += 0x9e3779b97f4a7c15ULL;
    value = *state;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

static int absolute_value(int value) {
    if (value < 0) {
        return -value;
    }

    return value;
}

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
    if (getPiece(&state->board, whiteKingStart).type == KING &&
        getPiece(&state->board, whiteKingStart).color == WHITE && history_touches_square(state, whiteKingStart) == 0) {
        if (getPiece(&state->board, whiteKingsideRookStart).type == ROOK &&
            getPiece(&state->board, whiteKingsideRookStart).color == WHITE &&
            history_touches_square(state, whiteKingsideRookStart) == 0) {
            rights |= HASH_CASTLE_WHITE_KINGSIDE;
        }
        if (getPiece(&state->board, whiteQueensideRookStart).type == ROOK &&
            getPiece(&state->board, whiteQueensideRookStart).color == WHITE &&
            history_touches_square(state, whiteQueensideRookStart) == 0) {
            rights |= HASH_CASTLE_WHITE_QUEENSIDE;
        }
    }

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

    lastMove = getMove((MoveList *)&state->moveHistory, state->moveHistory.count - 1);
    if (lastMove == NULL || lastMove->movedPiece.type != ANT || lastMove->from.col != lastMove->to.col) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    delta = absolute_value(lastMove->to.row - lastMove->from.row);
    if (delta != 2) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    file = lastMove->to.col;
    enemyColor = (lastMove->movedPiece.color == WHITE) ? BLACK : WHITE;
    if (state->currentTurn != enemyColor) {
        return HASH_NO_EN_PASSANT_FILE;
    }

    direction = (state->currentTurn == WHITE) ? -1 : 1;
    target = createPosition(lastMove->to.row + direction, file);
    if (!isValidPosition(target) || getPiece(&state->board, target).type != EMPTY_PIECE) {
        return HASH_NO_EN_PASSANT_FILE;
    }

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
    case PROMOTION_ANTEATER:
        return ANTEATER;
    case NO_SPECIAL_MOVE:
    case CASTLING_KINGSIDE:
    case CASTLING_QUEENSIDE:
    case EN_PASSANT:
    case ANTEATER_CAPTURE:
    default:
        return EMPTY_PIECE;
    }
}

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

    g_sideToMoveKey = splitmix64(&seed);
    for (index = 0; index < 4; ++index) {
        g_castlingKeys[index] = splitmix64(&seed);
    }
    for (index = 0; index < COLS; ++index) {
        g_enPassantKeys[index] = splitmix64(&seed);
    }

    g_zobristReady = 1;
}

int deriveHashState(const GameState *state, HashState *out) {
    uint64_t value;
    int row;
    int col;

    if (state == NULL || out == NULL) {
        return 1;
    }

    initZobrist();
    value = 0;
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Position pos = createPosition(row, col);
            Piece piece = getPiece(&state->board, pos);

            value ^= piece_key(piece, pos);
        }
    }

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

uint64_t computeHash(const GameState *state) {
    HashState hashState;

    if (deriveHashState(state, &hashState) != 0) {
        return 0;
    }

    return hashState.value;
}

uint64_t updateHashMove(uint64_t hash, Move move) {
    uint64_t mix;

    mix = (uint64_t)(move.from.row & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.from.col & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.to.row & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.to.col & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.specialType & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.movedPiece.type & 0xff);
    mix = (mix << 8) ^ (uint64_t)(move.movedPiece.color & 0xff);
    mix ^= mix << 17;
    mix ^= mix >> 13;
    mix ^= mix << 7;
    return hash ^ mix;
}

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

    if (previous->enPassantFile != HASH_NO_EN_PASSANT_FILE) {
        value ^= g_enPassantKeys[previous->enPassantFile];
    }

    value ^= g_sideToMoveKey;
    value ^= piece_key(move.movedPiece, move.from);
    for (captureIndex = 0; captureIndex < move.captureCount; ++captureIndex) {
        value ^= piece_key(move.captures[captureIndex].piece, move.captures[captureIndex].pos);
    }

    movedToPiece = piece_after_move(move);
    value ^= piece_key(movedToPiece, move.to);

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

    newRights = previous->castlingRights;
    if (move.movedPiece.type == KING) {
        if (move.movedPiece.color == WHITE) {
            newRights &= (unsigned char)~(HASH_CASTLE_WHITE_KINGSIDE | HASH_CASTLE_WHITE_QUEENSIDE);
        } else {
            newRights &= (unsigned char)~(HASH_CASTLE_BLACK_KINGSIDE | HASH_CASTLE_BLACK_QUEENSIDE);
        }
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
    if (move.movedPiece.type == ANT && move.from.col == move.to.col &&
        absolute_value(move.to.row - move.from.row) == 2) {
        int offset;
        int direction;
        Color enemyColor;
        Position target;

        enemyColor = (move.movedPiece.color == WHITE) ? BLACK : WHITE;
        if (after->currentTurn != enemyColor) {
            next->value = value;
            return 0;
        }

        direction = (after->currentTurn == WHITE) ? -1 : 1;
        target = createPosition(move.to.row + direction, move.to.col);
        if (!isValidPosition(target) || getPiece(&after->board, target).type != EMPTY_PIECE) {
            next->value = value;
            return 0;
        }

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

    if (next->enPassantFile != HASH_NO_EN_PASSANT_FILE) {
        value ^= g_enPassantKeys[next->enPassantFile];
    }

    next->value = value;
    return 0;
}
