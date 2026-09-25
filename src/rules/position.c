#include "internal.h"
#include <string.h>
#include <stdlib.h>
static uint64_t mix(uint64_t x) {
    x += UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}
static uint64_t piece_key(AcPiece p, int r, int c) {
    return p.type == AC_EMPTY_PIECE ? 0 : mix(1 + (unsigned)p.type + 7 * (p.color + 2 * (c + AC_COLS * r)));
}
static uint64_t rights_key(const AcPosition *p) {
    uint64_t h = mix(2048 + p->currentTurn) ^ mix(2050 + p->castlingRights);
    if (ac_is_valid_position(p->enPassant)) {
        AcPiece ant = ac_get_piece(&p->board, p->enPassant);
        int row = p->enPassant.row, col = p->enPassant.col;
        int direction = p->currentTurn == AC_WHITE ? -1 : 1;
        if (ant.type == AC_ANT && ant.color != p->currentTurn &&
            ac_get_piece(&p->board, ac_create_position(row + direction, col)).type == AC_EMPTY_PIECE) {
            for (int dc = -1; dc <= 1; dc += 2) {
                AcPiece neighbor = ac_get_piece(&p->board, ac_create_position(row, col + dc));
                if (neighbor.type == AC_ANT && neighbor.color == p->currentTurn) {
                    h ^= mix(2100 + col);
                    break;
                }
            }
        }
    }
    return h;
}
uint64_t ac_position_hash(const AcPosition *p) {
    if (!p)
        return 0;
    uint64_t h = rights_key(p);
    for (int r = 0; r < AC_ROWS; ++r)
        for (int c = 0; c < AC_COLS; ++c)
            h ^= piece_key(p->board.cells[r][c], r, c);
    return h;
}
void ac_position_init(AcPosition *p) {
    if (!p)
        return;
    memset(p, 0, sizeof(*p));
    ac_init_board(&p->board);
    p->currentTurn = AC_WHITE;
    p->castlingRights = 15;
    p->enPassant = ac_create_position(-1, -1);
    p->hash = ac_position_hash(p);
}
static void touch(AcPosition *p, AcSquare q) {
    int shift = q.row == 7 ? 0 : 2;
    if (q.row != 0 && q.row != 7)
        return;
    if (q.col == 5)
        p->castlingRights &= ~(3 << shift);
    if (q.col == 9)
        p->castlingRights &= ~(1 << shift);
    if (q.col == 0)
        p->castlingRights &= ~(2 << shift);
}
AcStatus ac_position_make(AcPosition *p, AcMove m, AcUndo *u) {
    if (!p || !u || !ac_is_valid_position(m.from) || !ac_is_valid_position(m.to) || m.captureCount < 0 ||
        m.captureCount > AC_MAX_CHAIN || m.pathLength < 0 || m.pathLength > AC_MAX_CHAIN)
        return AC_INVALID_ARGUMENT;
    AcPiece piece = ac_get_piece(&p->board, m.from);
    if (piece.type == AC_EMPTY_PIECE || piece.type != m.movedPiece.type || piece.color != p->currentTurn ||
        piece.color != m.movedPiece.color)
        return AC_ILLEGAL_MOVE;
    u->before = *p;
    ac_remove_piece(&p->board, m.from);
    touch(p, m.from);
    touch(p, m.to);
    for (int i = 0; i < m.captureCount; ++i) {
        ac_remove_piece(&p->board, m.captures[i].pos);
        touch(p, m.captures[i].pos);
    }
    if (m.specialType == AC_CASTLING_KINGSIDE || m.specialType == AC_CASTLING_QUEENSIDE) {
        int from = m.specialType == AC_CASTLING_KINGSIDE ? 9 : 0;
        int to = m.specialType == AC_CASTLING_KINGSIDE ? m.to.col - 1 : m.to.col + 1;
        AcSquare rook = ac_create_position(m.from.row, from);
        ac_set_piece(&p->board, ac_create_position(m.from.row, to), ac_get_piece(&p->board, rook));
        ac_remove_piece(&p->board, rook);
        touch(p, rook);
    }
    if (ac_is_promotion_special_move(m.specialType)) {
        const AcPieceType promotions[] = {AC_QUEEN, AC_ROOK, AC_BISHOP, AC_KNIGHT};
        piece.type = promotions[m.specialType - AC_PROMOTION_QUEEN];
    }
    ac_set_piece(&p->board, m.to, piece);
    p->enPassant = m.movedPiece.type == AC_ANT && abs(m.to.row - m.from.row) == 2 ? m.to : ac_create_position(-1, -1);
    p->currentTurn = p->currentTurn == AC_WHITE ? AC_BLACK : AC_WHITE;
    ++p->moveCount;
    p->hash = u->before.hash ^ rights_key(&u->before) ^ rights_key(p);
    for (int r = 0; r < AC_ROWS; ++r)
        for (int c = 0; c < AC_COLS; ++c)
            p->hash ^= piece_key(u->before.board.cells[r][c], r, c) ^ piece_key(p->board.cells[r][c], r, c);
    return AC_OK;
}
AcStatus ac_position_unmake(AcPosition *p, const AcUndo *u) {
    if (!p || !u)
        return AC_INVALID_ARGUMENT;
    *p = u->before;
    return AC_OK;
}
static int same_move(AcMove a, AcMove b) {
    if (a.movedPiece.type != b.movedPiece.type || a.movedPiece.color != b.movedPiece.color)
        return 0;
    if (!ac_position_equal(a.from, b.from) || !ac_position_equal(a.to, b.to) || a.specialType != b.specialType ||
        a.captureCount != b.captureCount || a.pathLength != b.pathLength)
        return 0;
    for (int i = 0; i < a.captureCount; ++i)
        if (!ac_position_equal(a.captures[i].pos, b.captures[i].pos) ||
            a.captures[i].piece.type != b.captures[i].piece.type ||
            a.captures[i].piece.color != b.captures[i].piece.color)
            return 0;
    for (int i = 0; i < a.pathLength; ++i)
        if (!ac_position_equal(a.path[i], b.path[i]))
            return 0;
    return 1;
}
AcStatus ac_position_apply(AcPosition *p, AcMove m, AcUndo *u) {
    if (!p || !u)
        return AC_INVALID_ARGUMENT;
    AcMoveList *list = malloc(sizeof(*list));
    if (!list)
        return AC_OUT_OF_MEMORY;
    AcStatus status = ac_generate_legal_moves_for_position(p, m.from, list);
    if (status != AC_OK) {
        free(list);
        return status;
    }
    status = AC_ILLEGAL_MOVE;
    for (int i = 0; i < list->count; ++i)
        if (same_move(m, list->moves[i])) {
            status = ac_position_make(p, list->moves[i], u);
            break;
        }
    free(list);
    return status;
}
