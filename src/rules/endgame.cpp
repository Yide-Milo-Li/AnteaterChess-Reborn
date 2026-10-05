#include "internal.hpp"
#include "anteater/memory.hpp"

namespace ac {
int is_in_check(const Position *s, Color c) {
    if (!s || (c != Color::White && c != Color::Black))
        return 0;
    for (int r = 0; r < Rows; ++r)
        for (int k = 0; k < Columns; ++k) {
            Piece p = s->board.cells[r][k];
            if (p.type == PieceType::King && p.color == c)
                return square_attacked(&s->board, create_position(r, k),
                                       c == Color::White ? Color::Black : Color::White);
        }
    return 0;
}
int is_insufficient_material(const Position *state) {
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

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece = get_piece(&state->board, create_position(row, col));
            int squareColor;

            /* Kings do not count toward mating material, and empty squares are
             * ignored entirely. */
            if (piece.type == PieceType::Empty || piece.type == PieceType::King) {
                continue;
            }

            ++totalNonKingPieces;

            /* Count each remaining piece type so the draw rules can make a
             * simple decision after the full board scan is done. */
            if (piece.type == PieceType::Ant) {
                ++totalAnts;
            } else if (piece.type == PieceType::Rook) {
                ++totalRooks;
            } else if (piece.type == PieceType::Queen) {
                ++totalQueens;
            } else if (piece.type == PieceType::Bishop) {
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
            } else if (piece.type == PieceType::Knight) {
                ++totalKnights;
            } else if (piece.type == PieceType::Anteater) {
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

Status position_result(const Position *s, GameResult *result, std::pmr::memory_resource *resource) {
    if (!resource)
        return Status::InvalidArgument;
    try {
        if (!s || !result)
            return Status::InvalidArgument;
        auto workspace = detail::make_owned<MoveList>(resource);
        MoveList *moves = workspace.get();
        Status status = static_cast<Status>(generate_legal_moves(s, moves));
        int count = moves->count;
        if (status != Status::Ok)
            return status;
        *result = GameResult::None;
        if (count == 0)
            *result = is_in_check(s, s->currentTurn)
                          ? (s->currentTurn == Color::White ? GameResult::BlackWin : GameResult::WhiteWin)
                          : GameResult::Draw;
        else if (is_insufficient_material(s))
            *result = GameResult::Draw;
        return Status::Ok;

    } catch (const std::bad_alloc &) {
        return Status::OutOfMemory;
    }
}
} // namespace ac
