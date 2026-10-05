#include "internal.hpp"
#include <string.h>
#include "anteater/memory.hpp"

namespace ac {
static uint64_t mix(uint64_t x) {
    x += UINT64_C(0x9e3779b97f4a7c15);
    x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
    return x ^ (x >> 31);
}
static uint64_t piece_key(Piece p, int r, int c) {
    return p.type == PieceType::Empty ? 0 : mix(1 + (unsigned)p.type + 7 * (value(p.color) + 2 * (c + Columns * r)));
}
static uint64_t rights_key(const Position *p) {
    uint64_t h = mix(2048 + value(p->currentTurn)) ^ mix(2050 + p->castlingRights);
    if (is_valid_position(p->enPassant)) {
        Piece ant = get_piece(&p->board, p->enPassant);
        int row = p->enPassant.row, col = p->enPassant.col;
        int direction = p->currentTurn == Color::White ? -1 : 1;
        if (ant.type == PieceType::Ant && ant.color != p->currentTurn &&
            get_piece(&p->board, create_position(row + direction, col)).type == PieceType::Empty) {
            for (int dc = -1; dc <= 1; dc += 2) {
                Piece neighbor = get_piece(&p->board, create_position(row, col + dc));
                if (neighbor.type == PieceType::Ant && neighbor.color == p->currentTurn) {
                    h ^= mix(2100 + col);
                    break;
                }
            }
        }
    }
    return h;
}
uint64_t position_hash(const Position *p) {
    if (!p)
        return 0;
    uint64_t h = rights_key(p);
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Columns; ++c)
            h ^= piece_key(p->board.cells[r][c], r, c);
    return h;
}
void position_init(Position *p) {
    if (!p)
        return;
    memset(p, 0, sizeof(*p));
    init_board(&p->board);
    p->currentTurn = Color::White;
    p->castlingRights = 15;
    p->enPassant = create_position(-1, -1);
    p->hash = position_hash(p);
}
static void touch(Position *p, Square q) {
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
Status position_make(Position *p, Move m, Undo *u) {
    if (!p || !u || !is_valid_position(m.from) || !is_valid_position(m.to) || m.captureCount < 0 ||
        m.captureCount > MaxChain || m.pathLength < 0 || m.pathLength > MaxChain)
        return Status::InvalidArgument;
    Piece piece = get_piece(&p->board, m.from);
    if (piece.type == PieceType::Empty || piece.type != m.movedPiece.type || piece.color != p->currentTurn ||
        piece.color != m.movedPiece.color)
        return Status::IllegalMove;
    u->before = *p;
    remove_piece(&p->board, m.from);
    touch(p, m.from);
    touch(p, m.to);
    for (int i = 0; i < m.captureCount; ++i) {
        remove_piece(&p->board, m.captures[i].pos);
        touch(p, m.captures[i].pos);
    }
    if (m.specialType == SpecialMove::CastlingKingside || m.specialType == SpecialMove::CastlingQueenside) {
        int from = m.specialType == SpecialMove::CastlingKingside ? 9 : 0;
        int to = m.specialType == SpecialMove::CastlingKingside ? m.to.col - 1 : m.to.col + 1;
        Square rook = create_position(m.from.row, from);
        set_piece(&p->board, create_position(m.from.row, to), get_piece(&p->board, rook));
        remove_piece(&p->board, rook);
        touch(p, rook);
    }
    if (is_promotion_special_move(m.specialType)) {
        const PieceType promotions[] = {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight};
        piece.type = promotions[value(m.specialType) - value(SpecialMove::PromotionQueen)];
    }
    set_piece(&p->board, m.to, piece);
    p->enPassant =
        m.movedPiece.type == PieceType::Ant && abs(m.to.row - m.from.row) == 2 ? m.to : create_position(-1, -1);
    p->currentTurn = p->currentTurn == Color::White ? Color::Black : Color::White;
    ++p->moveCount;
    p->hash = u->before.hash ^ rights_key(&u->before) ^ rights_key(p);
    for (int r = 0; r < Rows; ++r)
        for (int c = 0; c < Columns; ++c)
            p->hash ^= piece_key(u->before.board.cells[r][c], r, c) ^ piece_key(p->board.cells[r][c], r, c);
    return Status::Ok;
}
Status position_unmake(Position *p, const Undo *u) {
    if (!p || !u)
        return Status::InvalidArgument;
    *p = u->before;
    return Status::Ok;
}
static int same_move(Move a, Move b) {
    if (a.movedPiece.type != b.movedPiece.type || a.movedPiece.color != b.movedPiece.color)
        return 0;
    if (!position_equal(a.from, b.from) || !position_equal(a.to, b.to) || a.specialType != b.specialType ||
        a.captureCount != b.captureCount || a.pathLength != b.pathLength)
        return 0;
    for (int i = 0; i < a.captureCount; ++i)
        if (!position_equal(a.captures[i].pos, b.captures[i].pos) ||
            a.captures[i].piece.type != b.captures[i].piece.type ||
            a.captures[i].piece.color != b.captures[i].piece.color)
            return 0;
    for (int i = 0; i < a.pathLength; ++i)
        if (!position_equal(a.path[i], b.path[i]))
            return 0;
    return 1;
}
Status position_apply(Position *p, Move m, Undo *u, std::pmr::memory_resource *resource) {
    if (!resource)
        return Status::InvalidArgument;
    try {
        if (!p || !u)
            return Status::InvalidArgument;
        auto workspace = detail::make_owned<MoveList>(resource);
        MoveList *list = workspace.get();
        Status status = static_cast<Status>(generate_legal_moves_for_position(p, m.from, list));
        if (status != Status::Ok) {
            return status;
        }
        status = Status::IllegalMove;
        for (int i = 0; i < list->count; ++i)
            if (same_move(m, list->moves[i])) {
                status = position_make(p, list->moves[i], u);
                break;
            }
        return status;

    } catch (const std::bad_alloc &) {
        return Status::OutOfMemory;
    }
}
} // namespace ac
