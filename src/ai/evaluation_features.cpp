#include "anteater/rules.hpp"
#include "score_constants.hpp"
#include "mobility.hpp"
#include "evaluation_features.hpp"

namespace ac {

static int ai_ant_supports_square(const Board *board, Color color, Square target) {
    int supportRow;
    int offset;

    supportRow = target.row + ((color == Color::White) ? 1 : -1);
    for (offset = -1; offset <= 1; offset += 2) {
        Square support = create_position(supportRow, target.col + offset);
        Piece piece;

        if (!is_valid_position(support)) {
            continue;
        }

        piece = get_piece(board, support);
        if (piece.type == PieceType::Ant && piece.color == color) {
            return 1;
        }
    }

    return 0;
}

static int ai_enemy_ant_can_attack_square(const Board *board, Color color, Square target) {
    Color enemyColor;
    int enemyRow;
    int offset;

    enemyColor = (color == Color::White) ? Color::Black : Color::White;
    enemyRow = target.row + ((color == Color::White) ? -1 : 1);
    for (offset = -1; offset <= 1; offset += 2) {
        Square attacker = create_position(enemyRow, target.col + offset);
        Piece piece;

        if (!is_valid_position(attacker)) {
            continue;
        }

        piece = get_piece(board, attacker);
        if (piece.type == PieceType::Ant && piece.color == enemyColor) {
            return 1;
        }
    }

    return 0;
}

int ai_evaluate_pawns(const Position *state, Color color, const int antFiles[2][Columns], int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece = get_piece(&state->board, create_position(row, col));
            int adjacentFriends;
            int isPassed;
            int step;
            int advance;
            int passedBonus;

            // Get all alley ants
            if (piece.type != PieceType::Ant || piece.color != color) {
                continue;
            }

            // antFiles[enum_index(color)][col]: the number of color ants in the same file
            // Double pawns penalty
            if (antFiles[enum_index(color)][col] > 1) {
                score -= 12 * (antFiles[enum_index(color)][col] - 1);
            }

            // penalty for isolated ants and bonus for connected ants
            adjacentFriends = 0;
            if (col > 0 && antFiles[enum_index(color)][col - 1] > 0) {
                adjacentFriends = 1;
            }
            if (col + 1 < Columns && antFiles[enum_index(color)][col + 1] > 0) {
                adjacentFriends = 1;
            }
            if (!adjacentFriends) {
                score -= 10;
            } else {
                score += 6;
            }

            // check passed pawn
            isPassed = 1;
            if (color == Color::White) {
                for (step = row - 1; step >= 0 && isPassed; --step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        Piece enemy;

                        if (file < 0 || file >= Columns) {
                            continue;
                        }

                        enemy = get_piece(&state->board, create_position(step, file));
                        if (enemy.type == PieceType::Ant && enemy.color == Color::Black) {
                            isPassed = 0;
                            break;
                        }
                    }
                }
                advance = Rows - 1 - row;
            } else {
                for (step = row + 1; step < Rows && isPassed; ++step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        Piece enemy;

                        if (file < 0 || file >= Columns) {
                            continue;
                        }

                        enemy = get_piece(&state->board, create_position(step, file));
                        if (enemy.type == PieceType::Ant && enemy.color == Color::White) {
                            isPassed = 0;
                            break;
                        }
                    }
                }
                advance = row;
            }

            // bonus for passed pawn
            if (isPassed) {
                passedBonus = 18 + advance * 12;
                if (phase <= 12) {
                    passedBonus += 10 + advance * 4;
                }
                if (adjacentFriends) {
                    passedBonus += 8;
                }
                score += passedBonus;
            }
        }
    }

    return score;
}

int ai_evaluate_rook_and_queen_files(const Position *state, Color color, const int antFiles[2][Columns], int phase) {
    int score;
    int row;
    int col;
    Color enemyColor;

    (void)phase;

    score = 0;
    enemyColor = (color == Color::White) ? Color::Black : Color::White;
    // iterate over all rooks and queens
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece = get_piece(&state->board, create_position(row, col));

            if (piece.color != color || (piece.type != PieceType::Rook && piece.type != PieceType::Queen)) {
                continue;
            }

            // If file without any ants , add bonus
            if (antFiles[enum_index(color)][col] == 0 && antFiles[enum_index(enemyColor)][col] == 0) {
                score += (piece.type == PieceType::Rook) ? 20 : 8;
                // If file without any alley ants , add bonus
            } else if (antFiles[enum_index(color)][col] == 0) {
                score += (piece.type == PieceType::Rook) ? 12 : 4;
            }

            // Bonus for rook on 7th rank
            if (piece.type == PieceType::Rook) {
                if ((color == Color::White && row == 1) || (color == Color::Black && row == 6)) {
                    score += 14;
                }
            }
        }
    }

    return score;
}

int ai_evaluate_king_safety(const Position *state, Color color, Square kingPos, int phase) {
    int score;
    int homeRow;
    int shieldRow;
    int colOffset;

    if (!is_valid_position(kingPos)) {
        return 0;
    }

    score = 0;
    homeRow = (color == Color::White) ? 7 : 0;
    // Bonus for Castling
    if (kingPos.row == homeRow && (kingPos.col == 3 || kingPos.col == 7)) {
        score += 32;
        // game ending penalty for king on default position
    } else if (phase >= 18 && kingPos.row == homeRow && kingPos.col == 5) {
        score -= 24;
    }

    // Pawn Shield
    shieldRow = kingPos.row + ((color == Color::White) ? -1 : 1);
    for (colOffset = -1; colOffset <= 1; ++colOffset) {
        Square shield = create_position(shieldRow, kingPos.col + colOffset);
        Piece occupant;

        if (!is_valid_position(shield)) {
            continue;
        }

        occupant = get_piece(&state->board, shield);
        if (occupant.type == PieceType::Ant && occupant.color == color) {
            score += 12;
        } else {
            score -= 8;
        }
    }

    // mobility bonus in end game
    if (phase <= 10) {
        score += ai_count_king_mobility(&state->board, kingPos, create_piece(PieceType::King, color)) * 2;
    }

    return score;
}

int ai_evaluate_development(const Position *state, Color color, int phase) {
    int score;
    int homeRow;
    int undevelopedMinors;
    int col;

    // Only evaluate in opening and midgame
    if (state->moveCount > 20 || phase < 12) {
        return 0;
    }

    score = 0;
    undevelopedMinors = 0;
    homeRow = (color == Color::White) ? 7 : 0;

    for (col = 0; col < Columns; ++col) {
        Piece piece = get_piece(&state->board, create_position(homeRow, col));

        if (piece.color != color) {
            continue;
        }

        // penalty for undeveloped knights and bishops
        if (piece.type == PieceType::Knight || piece.type == PieceType::Bishop) {
            ++undevelopedMinors;
            score -= 12;
        }
    }

    // penalty for queen when there are 2 or more undeveloped minors
    if (undevelopedMinors >= 2) {
        Piece queen = get_piece(&state->board, create_position(homeRow, 4));

        if (queen.type != PieceType::Queen || queen.color != color) {
            score -= 10;
        }
    }

    // bonus for castling
    if (get_piece(&state->board, create_position(homeRow, 3)).type == PieceType::King &&
        get_piece(&state->board, create_position(homeRow, 3)).color == color) {
        score += 8;
    }
    if (get_piece(&state->board, create_position(homeRow, 7)).type == PieceType::King &&
        get_piece(&state->board, create_position(homeRow, 7)).color == color) {
        score += 8;
    }

    return score;
}

int ai_evaluate_anteater_threats(const Board *board, Color color) {
    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece = board->cells[row][col];
            int direction;

            if (piece.type != PieceType::Anteater || piece.color != color) {
                continue;
            }

            for (direction = 0; direction < 4; ++direction) {
                int r2 = row + dr[direction];
                int c2 = col + dc[direction];
                int chainLength;

                chainLength = 0;
                while (r2 >= 0 && r2 < Rows && c2 >= 0 && c2 < Columns) {
                    Piece target = board->cells[r2][c2];

                    if (target.type == PieceType::Ant && target.color != color) {
                        ++chainLength;
                        r2 += dr[direction];
                        c2 += dc[direction];
                        continue;
                    }
                    break;
                }

                if (chainLength >= 2) {
                    score += 10 + chainLength * 15;
                }
            }
        }
    }

    return score;
}

int ai_evaluate_outposts(const Board *board, Color color, int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Square pos = create_position(row, col);
            Piece piece = get_piece(board, pos);
            int bonus;

            if (piece.color != color) {
                continue;
            }
            if (piece.type != PieceType::Knight && piece.type != PieceType::Bishop &&
                piece.type != PieceType::Anteater) {
                continue;
            }
            if ((color == Color::White && row >= 5) || (color == Color::Black && row <= 2)) {
                continue;
            }
            if (!ai_ant_supports_square(board, color, pos)) {
                continue;
            }
            if (ai_enemy_ant_can_attack_square(board, color, pos)) {
                continue;
            }

            if (piece.type == PieceType::Knight) {
                bonus = 16;
            } else if (piece.type == PieceType::Bishop) {
                bonus = 12;
            } else {
                bonus = 14;
            }

            // bonus for outpost in center
            if (col >= 3 && col <= 6) {
                bonus += 4;
            }

            // reduce score in end game
            if (phase <= 8) {
                bonus /= 2;
            }

            score += bonus;
        }
    }

    return score;
}

} // namespace ac
