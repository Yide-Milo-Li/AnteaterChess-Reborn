#include "anteater/rules.h"
#include "score_constants.h"
#include "mobility.h"
#include "evaluation_features.h"

static int ac_ai_ant_supports_square(const AcBoard *board, AcColor color, AcSquare target) {
    int supportRow;
    int offset;

    supportRow = target.row + ((color == AC_WHITE) ? 1 : -1);
    for (offset = -1; offset <= 1; offset += 2) {
        AcSquare support = ac_create_position(supportRow, target.col + offset);
        AcPiece piece;

        if (!ac_is_valid_position(support)) {
            continue;
        }

        piece = ac_get_piece(board, support);
        if (piece.type == AC_ANT && piece.color == color) {
            return 1;
        }
    }

    return 0;
}

static int ac_ai_enemy_ant_can_attack_square(const AcBoard *board, AcColor color, AcSquare target) {
    AcColor enemyColor;
    int enemyRow;
    int offset;

    enemyColor = (color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    enemyRow = target.row + ((color == AC_WHITE) ? -1 : 1);
    for (offset = -1; offset <= 1; offset += 2) {
        AcSquare attacker = ac_create_position(enemyRow, target.col + offset);
        AcPiece piece;

        if (!ac_is_valid_position(attacker)) {
            continue;
        }

        piece = ac_get_piece(board, attacker);
        if (piece.type == AC_ANT && piece.color == enemyColor) {
            return 1;
        }
    }

    return 0;
}

int ac_ai_evaluate_pawns(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS], int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));
            int adjacentFriends;
            int isPassed;
            int step;
            int advance;
            int passedBonus;

            // Get all alley ants
            if (piece.type != AC_ANT || piece.color != color) {
                continue;
            }

            // antFiles[color][col]: the number of color ants in the same file
            // Double pawns penalty
            if (antFiles[color][col] > 1) {
                score -= 12 * (antFiles[color][col] - 1);
            }

            // penalty for isolated ants and bonus for connected ants
            adjacentFriends = 0;
            if (col > 0 && antFiles[color][col - 1] > 0) {
                adjacentFriends = 1;
            }
            if (col + 1 < AC_COLS && antFiles[color][col + 1] > 0) {
                adjacentFriends = 1;
            }
            if (!adjacentFriends) {
                score -= 10;
            } else {
                score += 6;
            }

            // check passed pawn
            isPassed = 1;
            if (color == AC_WHITE) {
                for (step = row - 1; step >= 0 && isPassed; --step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        AcPiece enemy;

                        if (file < 0 || file >= AC_COLS) {
                            continue;
                        }

                        enemy = ac_get_piece(&state->board, ac_create_position(step, file));
                        if (enemy.type == AC_ANT && enemy.color == AC_BLACK) {
                            isPassed = 0;
                            break;
                        }
                    }
                }
                advance = AC_ROWS - 1 - row;
            } else {
                for (step = row + 1; step < AC_ROWS && isPassed; ++step) {
                    int file;

                    for (file = col - 1; file <= col + 1; ++file) {
                        AcPiece enemy;

                        if (file < 0 || file >= AC_COLS) {
                            continue;
                        }

                        enemy = ac_get_piece(&state->board, ac_create_position(step, file));
                        if (enemy.type == AC_ANT && enemy.color == AC_WHITE) {
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

int ac_ai_evaluate_rook_and_queen_files(const AcPosition *state, AcColor color, const int antFiles[2][AC_COLS],
                                        int phase) {
    int score;
    int row;
    int col;
    AcColor enemyColor;

    (void)phase;

    score = 0;
    enemyColor = (color == AC_WHITE) ? AC_BLACK : AC_WHITE;
    // iterate over all rooks and queens
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));

            if (piece.color != color || (piece.type != AC_ROOK && piece.type != AC_QUEEN)) {
                continue;
            }

            // If file without any ants , add bonus
            if (antFiles[color][col] == 0 && antFiles[enemyColor][col] == 0) {
                score += (piece.type == AC_ROOK) ? 20 : 8;
                // If file without any alley ants , add bonus
            } else if (antFiles[color][col] == 0) {
                score += (piece.type == AC_ROOK) ? 12 : 4;
            }

            // Bonus for rook on 7th rank
            if (piece.type == AC_ROOK) {
                if ((color == AC_WHITE && row == 1) || (color == AC_BLACK && row == 6)) {
                    score += 14;
                }
            }
        }
    }

    return score;
}

int ac_ai_evaluate_king_safety(const AcPosition *state, AcColor color, AcSquare kingPos, int phase) {
    int score;
    int homeRow;
    int shieldRow;
    int colOffset;

    if (!ac_is_valid_position(kingPos)) {
        return 0;
    }

    score = 0;
    homeRow = (color == AC_WHITE) ? 7 : 0;
    // Bonus for Castling
    if (kingPos.row == homeRow && (kingPos.col == 3 || kingPos.col == 7)) {
        score += 32;
        // game ending penalty for king on default position
    } else if (phase >= 18 && kingPos.row == homeRow && kingPos.col == 5) {
        score -= 24;
    }

    // Pawn Shield
    shieldRow = kingPos.row + ((color == AC_WHITE) ? -1 : 1);
    for (colOffset = -1; colOffset <= 1; ++colOffset) {
        AcSquare shield = ac_create_position(shieldRow, kingPos.col + colOffset);
        AcPiece occupant;

        if (!ac_is_valid_position(shield)) {
            continue;
        }

        occupant = ac_get_piece(&state->board, shield);
        if (occupant.type == AC_ANT && occupant.color == color) {
            score += 12;
        } else {
            score -= 8;
        }
    }

    // mobility bonus in end game
    if (phase <= 10) {
        score += ac_ai_count_king_mobility(&state->board, kingPos, ac_create_piece(AC_KING, color)) * 2;
    }

    return score;
}

int ac_ai_evaluate_development(const AcPosition *state, AcColor color, int phase) {
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
    homeRow = (color == AC_WHITE) ? 7 : 0;

    for (col = 0; col < AC_COLS; ++col) {
        AcPiece piece = ac_get_piece(&state->board, ac_create_position(homeRow, col));

        if (piece.color != color) {
            continue;
        }

        // penalty for undeveloped knights and bishops
        if (piece.type == AC_KNIGHT || piece.type == AC_BISHOP) {
            ++undevelopedMinors;
            score -= 12;
        }
    }

    // penalty for queen when there are 2 or more undeveloped minors
    if (undevelopedMinors >= 2) {
        AcPiece queen = ac_get_piece(&state->board, ac_create_position(homeRow, 4));

        if (queen.type != AC_QUEEN || queen.color != color) {
            score -= 10;
        }
    }

    // bonus for castling
    if (ac_get_piece(&state->board, ac_create_position(homeRow, 3)).type == AC_KING &&
        ac_get_piece(&state->board, ac_create_position(homeRow, 3)).color == color) {
        score += 8;
    }
    if (ac_get_piece(&state->board, ac_create_position(homeRow, 7)).type == AC_KING &&
        ac_get_piece(&state->board, ac_create_position(homeRow, 7)).color == color) {
        score += 8;
    }

    return score;
}

int ac_ai_evaluate_anteater_threats(const AcBoard *board, AcColor color) {
    static const int dr[] = {-1, 1, 0, 0};
    static const int dc[] = {0, 0, -1, 1};
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = board->cells[row][col];
            int direction;

            if (piece.type != AC_ANTEATER || piece.color != color) {
                continue;
            }

            for (direction = 0; direction < 4; ++direction) {
                int r2 = row + dr[direction];
                int c2 = col + dc[direction];
                int chainLength;

                chainLength = 0;
                while (r2 >= 0 && r2 < AC_ROWS && c2 >= 0 && c2 < AC_COLS) {
                    AcPiece target = board->cells[r2][c2];

                    if (target.type == AC_ANT && target.color != color) {
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

int ac_ai_evaluate_outposts(const AcBoard *board, AcColor color, int phase) {
    int score;
    int row;
    int col;

    score = 0;
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare pos = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(board, pos);
            int bonus;

            if (piece.color != color) {
                continue;
            }
            if (piece.type != AC_KNIGHT && piece.type != AC_BISHOP && piece.type != AC_ANTEATER) {
                continue;
            }
            if ((color == AC_WHITE && row >= 5) || (color == AC_BLACK && row <= 2)) {
                continue;
            }
            if (!ac_ai_ant_supports_square(board, color, pos)) {
                continue;
            }
            if (ac_ai_enemy_ant_can_attack_square(board, color, pos)) {
                continue;
            }

            if (piece.type == AC_KNIGHT) {
                bonus = 16;
            } else if (piece.type == AC_BISHOP) {
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

