#include "ai/ai.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "core/board.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/endgame.h"
#include "gameplay/execution.h"
#include "gameplay/movegen.h"

#define AI_INF 100000000
#define AI_MATE 1000000
#define AI_Q_DEPTH 4 // Only for Alpha Version, Quiescence Depth

// Piece-Square Table
static const int PST_ANT[ROWS][COLS] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

static const int PST_KNIGHT[ROWS][COLS] = {
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  25,  30,  30,  25,  15,  5,   -30},
    {-30, 5,   15,  20,  20,  20,  20,  15,  5,   -30},
    {-40, -20, 0,   5,   10,  10,  5,   0,   -20, -40},
    {-50, -40, -30, -30, -30, -30, -30, -30, -40, -50}
};

static const int PST_BISHOP[ROWS][COLS] = {
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20},
    {-10, 10,  0,   0,   5,   5,   0,   0,   10,  -10},
    {-10, 10,  10,  10,  10,  10,  10,  10,  10,  -10},
    {-10, 0,   15,  20,  20,  20,  20,  15,  0,   -10},
    {-10, 5,   15,  20,  20,  20,  20,  15,  5,   -10},
    {-10, 0,   10,  15,  15,  15,  15,  10,  0,   -10},
    {-10, 5,   0,   0,   0,   0,   0,   0,   5,   -10},
    {-20, -10, -10, -10, -10, -10, -10, -10, -10, -20}
};

static const int PST_ROOK[ROWS][COLS] = {
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 },
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {-5, 0,  0,  0,  0,  0,  0,  0,  0,  -5},
    {10, 15, 15, 15, 15, 15, 15, 15, 15, 10},
    {0,  0,  5,  10, 10, 10, 10, 5,  0,  0 }
};

static const int PST_QUEEN[ROWS][COLS] = {
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20},
    {-10, 0,   5,   0,  0,  0,  0,  5,   0,   -10},
    {-10, 5,   5,   5,  5,  5,  5,  5,   5,   -10},
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-5,  0,   5,   10, 10, 10, 10, 5,   0,   -5 },
    {-10, 0,   5,   5,  5,  5,  5,  5,   0,   -10},
    {-10, 0,   0,   0,  0,  0,  0,  0,   0,   -10},
    {-20, -10, -10, -5, -5, -5, -5, -10, -10, -20}
};

static const int PST_KING_MID[ROWS][COLS] = {
    {20,  30,  10,  0,   0,   0,   0,   10,  30,  20 },
    {20,  20,  0,   -5,  -5,  -5,  -5,  0,   20,  20 },
    {-10, -20, -20, -20, -20, -20, -20, -20, -20, -10},
    {-20, -30, -30, -40, -40, -40, -40, -30, -30, -20},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -50, -50, -40, -40, -30}
};

static const int PST_ANTEATER[ROWS][COLS] = {
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {10, 10, 15, 20, 25, 25, 20, 15, 10, 10},
    {5,  5,  10, 15, 15, 15, 15, 10, 5,  5 },
    {0,  5,  5,  5,  5,  5,  5,  5,  5,  0 },
    {0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};

// Global variables for search
static clock_t g_search_start; // Start time of the search
static int g_time_limit_ms;    // Time limit in milliseconds
static int g_stop_search; // Flag to stop the search, 1 = stop, 0 = continue
static int g_nodes;       // Number of nodes visited

// Piece Value Tables
static int piece_value(PieceType type) {
    switch (type) {
    case ANT:
        return 100;
    case KNIGHT:
        return 320;
    case BISHOP:
        return 330;
    case ROOK:
        return 500;
    case QUEEN:
        return 900;
    case KING:
        return 20000;
    case ANTEATER:
        return 200;
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

/* Helper function to reverse the PST for white
   Return the corespond row for the piece*/
static int board_row_for_pst(Color color, int row) {
    return (color == WHITE) ? (ROWS - 1 - row) : row;
}

/* Helper function to get the PST bonus for a piece
   Return the bonus for the piece*/
static int pst_bonus(Piece piece, int row, int col) {
    int pst_row; // get the corespond row

    pst_row = board_row_for_pst(piece.color, row);
    switch (piece.type) {
    case ANT:
        return PST_ANT[pst_row][col];
    case KNIGHT:
        return PST_KNIGHT[pst_row][col];
    case BISHOP:
        return PST_BISHOP[pst_row][col];
    case ROOK:
        return PST_ROOK[pst_row][col];
    case QUEEN:
        return PST_QUEEN[pst_row][col];
    case KING:
        return PST_KING_MID[pst_row][col];
    case ANTEATER:
        return PST_ANTEATER[pst_row][col];
    case EMPTY_PIECE:
    default:
        return 0;
    }
}

// evaluate pawns
static int evaluate_pawns(const GameState *state, Color color) {
    int score = 0;
    int row;
    int col;

    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));
            // init. the pawn's parameter
            int file_has_neighbor = 0;
            int file2;
            int row2;
            int is_passed = 1;

            // Check if the piece is an ANT of the current color
            if (piece.type != ANT || piece.color != color) {
                continue;
            }

            for (row2 = 0; row2 < ROWS; ++row2) {
                if (row2 != row &&
                    getPiece(&state->board, createPosition(row2, col)).type ==
                        ANT &&
                    getPiece(&state->board, createPosition(row2, col)).color ==
                        color) {
                    score -= 15;
                    break;
                }
            } // Doubled Pawns

            // check left and right columns to see is it isolated
            for (file2 = col - 1; file2 <= col + 1; file2 += 2) {
                if (file2 < 0 || file2 >= COLS) {
                    continue;
                }
                for (row2 = 0; row2 < ROWS; ++row2) {
                    Piece other =
                        getPiece(&state->board, createPosition(row2, file2));
                    if (other.type == ANT && other.color == color) {
                        file_has_neighbor = 1;
                        break;
                    }
                }
                if (file_has_neighbor) {
                    break;
                }
            }
            // Isolated Pawns -10
            if (!file_has_neighbor) {
                score -= 10;
            }

            // Passed Pawns White
            if (color == WHITE) {
                for (row2 = row - 1; row2 >= 0 && is_passed; --row2) {
                    // check left and right columns to see is it passed
                    for (file2 = col - 1; file2 <= col + 1; ++file2) {
                        Piece enemy;
                        // check edge
                        if (file2 < 0 || file2 >= COLS) {
                            continue;
                        }
                        // find ememy ant-->not passed
                        enemy = getPiece(&state->board,
                                         createPosition(row2, file2));
                        if (enemy.type == ANT && enemy.color == BLACK) {
                            is_passed = 0;
                            break;
                        }
                    }
                }
                if (is_passed) {
                    score += 10 + (ROWS - 1 - row) * 10;
                }
                // Passed Pawns Black
            } else {
                for (row2 = row + 1; row2 < ROWS && is_passed; ++row2) {
                    for (file2 = col - 1; file2 <= col + 1; ++file2) {
                        Piece enemy;
                        if (file2 < 0 || file2 >= COLS) {
                            continue;
                        }
                        enemy = getPiece(&state->board,
                                         createPosition(row2, file2));
                        if (enemy.type == ANT && enemy.color == WHITE) {
                            is_passed = 0;
                            break;
                        }
                    }
                }
                if (is_passed) {
                    score += 10 + row * 10;
                }
            }
        }
    }
    return score;
}

// evaluate the board
static int evaluate_absolute(const GameState *state) {
    int score = 0;
    int white_bishops = 0;
    int black_bishops = 0;
    int row;
    int col;

    // iterate through the board
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            Piece piece = getPiece(&state->board, createPosition(row, col));
            int value;

            if (piece.type == EMPTY_PIECE) {
                continue;
            }
            // Value = piece value + table bonus
            value = piece_value(piece.type) + pst_bonus(piece, row, col);

            // add value to score
            if (piece.color == WHITE) {
                score += value;
                // count white bishops
                if (piece.type == BISHOP) {
                    ++white_bishops;
                }
            } else {
                score -= value;
                // count black bishops
                if (piece.type == BISHOP) {
                    ++black_bishops;
                }
            }
        }
    }

    if (white_bishops >= 2) {
        score += 30;
    }
    if (black_bishops >= 2) {
        score -= 30;
    }
    // add pawn evaluation
    score += evaluate_pawns(state, WHITE);
    score -= evaluate_pawns(state, BLACK);

    return score;
}

// evaluate the board relative to the current player
static int evaluate_relative(const GameState *state) {
    int absolute_score = evaluate_absolute(state);
    return (state->currentTurn == WHITE) ? absolute_score : -absolute_score;
}

// get the elapsed time in milliseconds
static int elapsed_ms(void) {
    clock_t now = clock();
    return (int)(((now - g_search_start) * 1000) / CLOCKS_PER_SEC);
}

// check if the time is up
static int time_is_up(void) {
    if (g_time_limit_ms <= 0) {
        return 0;
    }
    if ((g_nodes & 2047) == 0 && elapsed_ms() >= g_time_limit_ms) {
        g_stop_search = 1;
    }
    return g_stop_search;
}

// check if the move is a promotion move
static int is_promotion_move(const Move *move) {
    return move->specialType == PROMOTION_QUEEN ||
           move->specialType == PROMOTION_ROOK ||
           move->specialType == PROMOTION_BISHOP ||
           move->specialType == PROMOTION_KNIGHT;
}

// calculate the noisy move score, order the moves
static int noisy_move_score(const Move *move) {
    int score = 0;
    int capture_index;

    // add value for captures
    for (capture_index = 0; capture_index < move->captureCount;
         ++capture_index) {
        score += piece_value(move->captures[capture_index].piece.type) * 10;
    }
    // subtract value for moved piece
    score -= piece_value(move->movedPiece.type);

    // bonus for anteater capture
    if (move->specialType == ANTEATER_CAPTURE) {
        score += 500 + move->captureCount * 100;
    }
    // bonus for promotion
    if (is_promotion_move(move)) {
        score += 800;
    }

    return score;
}

// sort the moves by their noisy score
static void sort_moves(MoveList *list) {
    int i;

    for (i = 1; i < list->count; ++i) {
        Move key = list->moves[i];
        int key_score = noisy_move_score(&key);
        int j = i - 1;

        // insertion sort
        while (j >= 0 && noisy_move_score(&list->moves[j]) < key_score) {
            list->moves[j + 1] = list->moves[j];
            --j;
        }
        list->moves[j + 1] = key;
    }
}

// capture, promotion, anteater capture are noisy moves
static int is_noisy_move(const Move *move) {
    return move->captureCount > 0 || move->specialType == ANTEATER_CAPTURE ||
           is_promotion_move(move);
}

static int collect_fully_legal_moves(const GameState *state, MoveList *legal) {
    MoveList pseudo;
    int index;

    if (state == NULL || legal == NULL) {
        return 1;
    }

    initMoveList(legal);
    if (generateMoves(state, &pseudo) != 0) {
        return 1;
    }

    for (index = 0; index < pseudo.count; ++index) {
        Move candidate = pseudo.moves[index];
        GameState next = *state;
        Color moving_side = state->currentTurn;

        if (applyMove(&next, candidate) != 0) {
            continue;
        }
        // if got checked, then it's not a legal move
        if (isInCheck(&next, moving_side)) {
            continue;
        }
        addMove(legal, candidate);
    }

    return 0;
}

// collect noisy legal moves
static int collect_noisy_legal_moves(const GameState *state, MoveList *noisy) {
    MoveList all;
    int index;

    if (collect_fully_legal_moves(state, &all) != 0) {
        return 1;
    }

    initMoveList(noisy);
    for (index = 0; index < all.count; ++index) {
        if (is_noisy_move(&all.moves[index])) {
            addMove(noisy, all.moves[index]);
        }
    }

    return 0;
}

// quiescence search
static int quiescence(const GameState *state, int alpha, int beta, int ply,
                      int qdepth) {
    int stand_pat;
    MoveList noisy;
    int index;

    // avoid unused variable compile error
    (void)ply;

    // immediate return if time is up
    if (time_is_up()) {
        return evaluate_relative(state);
    }

    // increment node count
    ++g_nodes;
    // get static evaluation
    stand_pat = evaluate_relative(state);

    // beta cut-off when stand pat >= beta
    if (stand_pat >= beta) {
        return beta;
    }
    // update alpha when stand pat > alpha
    if (stand_pat > alpha) {
        alpha = stand_pat;
    }
    if (qdepth >= AI_Q_DEPTH) {
        return alpha;
    }

    // collect noisy legal moves, if there are no noisy moves, return alpha
    if (collect_noisy_legal_moves(state, &noisy) != 0) {
        return alpha;
    }

    // sort moves by noisy score
    sort_moves(&noisy);

    for (index = 0; index < noisy.count; ++index) {
        GameState next = *state;
        int score;

        // check if move is legal
        if (applyMove(&next, noisy.moves[index]) != 0) {
            continue;
        }

        // recursive call, main algorithm
        score = -quiescence(&next, -beta, -alpha, ply + 1, qdepth + 1);

        // if received stop signal, return alpha
        if (g_stop_search) {
            return alpha;
        }
        // beta cut-off when score >= beta
        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

// alpha-beta search
static int alpha_beta(const GameState *state, int depth, int alpha, int beta,
                      int ply) {
    MoveList legal;
    int best_score = -AI_INF;
    int index;

    // immediate return if time is up
    if (time_is_up()) {
        return evaluate_relative(state);
    }

    // increment node count
    ++g_nodes;

    if (collect_fully_legal_moves(state, &legal) != 0) {
        return evaluate_relative(state);
    }

    // Endgame detection
    if (legal.count == 0) {
        if (isInCheck(state, state->currentTurn)) {
            // checkmate and procrastinate as much as possible.
            return -AI_MATE + ply;
        }
        // stalemate
        return 0;
    }

    // depth limit reached, use quiescence search
    if (depth <= 0) {
        return quiescence(state, alpha, beta, ply, 0);
    }

    // sort moves by noisy score
    sort_moves(&legal);
    for (index = 0; index < legal.count; ++index) {
        GameState next = *state;
        int score;

        if (applyMove(&next, legal.moves[index]) != 0) {
            continue;
        }

        // recursive call, main algorithm of alpha-beta search
        score = -alpha_beta(&next, depth - 1, -beta, -alpha, ply + 1);

        // Only return the alpha which is fully computed
        if (g_stop_search) {
            return alpha;
        }

        // update best score
        if (score > best_score) {
            best_score = score;
        }
        // update alpha
        if (score > alpha) {
            alpha = score;
        }
        // Alpha-Beta Pruning!
        if (alpha >= beta) {
            break;
        }
    }

    return best_score;
}

static AIDifficulty difficulty_for_turn(const GameState *state) {
    AIDifficulty difficulty;

    if (state->currentTurn == WHITE) {
        difficulty = state->config.aiDifficultyWhite;
    } else {
        difficulty = state->config.aiDifficultyBlack;
    }

    // for robustnes
    if (difficulty == DIFFICULTY_NONE) {
        difficulty = DIFFICULTY_MEDIUM;
    }
    return difficulty;
}

// main alpha beta search depth
static int depth_for_difficulty(AIDifficulty difficulty) {
    switch (difficulty) {
    case DIFFICULTY_EASY:
        return 1;
    case DIFFICULTY_MEDIUM:
        return 2;
    case DIFFICULTY_HARD:
        return 4;
    case DIFFICULTY_NONE:
    default:
        return 2;
    }
}

static int time_budget_for_state(const GameState *state,
                                 AIDifficulty difficulty) {
    if (state->config.aiTimeLimit > 0) {
        return state->config.aiTimeLimit * 1000;
    }

    // incase aiTimeLimit is not set, use the default time budget
    switch (difficulty) {
    case DIFFICULTY_EASY:
        return 300;
    case DIFFICULTY_MEDIUM:
        return 1200;
    case DIFFICULTY_HARD:
        return 3000;
    case DIFFICULTY_NONE:
    default:
        return 1000;
    }
}

static int search_best_move(const GameState *state, int max_depth,
                            int max_time_ms, Move *best_move) {
    MoveList root_moves;
    int root_scores[MAX_MOVES];
    int depth;
    int index;
    Move current_best;
    int current_best_score;

    // robust check
    if (state == NULL || best_move == NULL) {
        return 1;
    }
    if (collect_fully_legal_moves(state, &root_moves) != 0) {
        return 1;
    }
    if (root_moves.count <= 0) {
        return 1;
    }

    // Special case: only one legal move
    *best_move = root_moves.moves[0];
    if (root_moves.count == 1) {
        return 0;
    }

    // initialize global variables
    memset(root_scores, 0, sizeof(root_scores));
    g_search_start = clock();
    g_time_limit_ms = max_time_ms;
    g_stop_search = 0;
    g_nodes = 0;

    current_best = root_moves.moves[0];
    current_best_score = -AI_INF;

    // iterative deepening
    for (depth = 1; depth <= max_depth; ++depth) {
        Move iteration_best = current_best;
        int iteration_best_score = -AI_INF;
        int alpha = -AI_INF;
        int beta = AI_INF;

        for (index = 0; index < root_moves.count; ++index) {
            GameState next = *state;
            int score;

            if (applyMove(&next, root_moves.moves[index]) != 0) {
                root_scores[index] = -AI_INF;
                continue;
            }

            score = -alpha_beta(&next, depth - 1, -beta, -alpha, 1);

            // stop if received stop signal
            if (g_stop_search) {
                break;
            }

            root_scores[index] = score;
            if (score > iteration_best_score) {
                iteration_best_score = score;
                iteration_best = root_moves.moves[index];
            }
            if (score > alpha) {
                alpha = score;
            }
        }

        // stop if received stop signal
        if (g_stop_search) {
            break;
        }

        // update best move
        current_best = iteration_best;
        current_best_score = iteration_best_score;
        *best_move = current_best;

        // sort moves by score
        for (index = 1; index < root_moves.count; ++index) {
            Move key_move = root_moves.moves[index];
            int key_score = root_scores[index];
            int j = index - 1;

            while (j >= 0 && root_scores[j] < key_score) {
                root_moves.moves[j + 1] = root_moves.moves[j];
                root_scores[j + 1] = root_scores[j];
                --j;
            }
            root_moves.moves[j + 1] = key_move;
            root_scores[j + 1] = key_score;
        }
    }

    (void)current_best_score;
    return 0;
}

/*Public API*/

int generateAIMove(const GameState *state, Move *move) {
    AIDifficulty difficulty;
    int max_depth;
    int max_time_ms;

    // robust check
    if (state == NULL || move == NULL) {
        return 1;
    }

    difficulty = difficulty_for_turn(state);
    max_depth = depth_for_difficulty(difficulty);
    max_time_ms = time_budget_for_state(state, difficulty);

    return search_best_move(state, max_depth, max_time_ms, move);
}

int generateHintMove(const GameState *state, Move *move) {
    int max_time_ms;

    // robust check
    if (state == NULL || move == NULL) {
        return 1;
    }

    max_time_ms = time_budget_for_state(state, DIFFICULTY_MEDIUM);
    return search_best_move(state, 2, max_time_ms, move);
}