#include "anteater/rules.h"
#include <string.h>
#include "score_constants.h"
#include "piece_tables.h"
#include "mobility.h"
#include "evaluation_features.h"
#include "evaluation.h"

int ac_ai_evaluate_absolute(const AcPosition *state) {
    int antFiles[2][AC_COLS];
    int bishopCount[2];
    AcSquare kingPos[2];
    int phase;
    int score;
    int row;
    int col;

    // init
    memset(antFiles, 0, sizeof(antFiles));
    memset(bishopCount, 0, sizeof(bishopCount));
    kingPos[AC_WHITE] = ac_create_position(-1, -1);
    kingPos[AC_BLACK] = ac_create_position(-1, -1);
    phase = 0;
    score = 0;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = ac_get_piece(&state->board, ac_create_position(row, col));

            if (piece.type == AC_EMPTY_PIECE) {
                continue;
            }

            if (piece.type == AC_ANT) {
                ++antFiles[piece.color][col];
            }
            if (piece.type == AC_BISHOP) {
                ++bishopCount[piece.color];
            }
            if (piece.type == AC_KING) {
                kingPos[piece.color] = ac_create_position(row, col);
            }

            phase += ac_ai_phase_value(piece.type);
        }
    }

    // robust
    if (phase > AI_MAX_PHASE) {
        phase = AI_MAX_PHASE;
    }

    // ac_ai_piece_value + PST + mobility
    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcSquare pos = ac_create_position(row, col);
            AcPiece piece = ac_get_piece(&state->board, pos);
            int value;

            if (piece.type == AC_EMPTY_PIECE) {
                continue;
            }

            value = ac_ai_piece_value(piece.type);
            value += ac_ai_pst_bonus(piece, row, col, phase);
            if (piece.type == AC_KNIGHT || piece.type == AC_ANTEATER) {
                value += ac_ai_piece_mobility(&state->board, pos, piece) * ac_ai_mobility_weight(piece.type);
            }

            if (piece.color == AC_WHITE) {
                score += value;
            } else {
                score -= value;
            }
        }
    }

    // bishop pair
    if (bishopCount[AC_WHITE] >= 2) {
        score += 36;
    }
    if (bishopCount[AC_BLACK] >= 2) {
        score -= 36;
    }

    score += ac_ai_evaluate_pawns(state, AC_WHITE, antFiles, phase);
    score -= ac_ai_evaluate_pawns(state, AC_BLACK, antFiles, phase);
    score += ac_ai_evaluate_rook_and_queen_files(state, AC_WHITE, antFiles, phase);
    score -= ac_ai_evaluate_rook_and_queen_files(state, AC_BLACK, antFiles, phase);
    score += ac_ai_evaluate_king_safety(state, AC_WHITE, kingPos[AC_WHITE], phase);
    score -= ac_ai_evaluate_king_safety(state, AC_BLACK, kingPos[AC_BLACK], phase);
    score += ac_ai_evaluate_development(state, AC_WHITE, phase);
    score -= ac_ai_evaluate_development(state, AC_BLACK, phase);
    score += ac_ai_evaluate_anteater_threats(&state->board, AC_WHITE);
    score -= ac_ai_evaluate_anteater_threats(&state->board, AC_BLACK);

    // outpost limitation
    if (phase >= 10) {
        score += ac_ai_evaluate_outposts(&state->board, AC_WHITE, phase);
        score -= ac_ai_evaluate_outposts(&state->board, AC_BLACK, phase);
    }

    return score;
}

int ac_ai_evaluate_relative(const AcPosition *state) {
    int absoluteScore;

    absoluteScore = ac_ai_evaluate_absolute(state);
    // tempo bonus
    if (state->currentTurn == AC_WHITE) {
        return absoluteScore + 1;
    }

    return -absoluteScore + 1;
}
