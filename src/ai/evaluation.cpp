#include "anteater/rules.hpp"
#include <string.h>
#include "score_constants.hpp"
#include "piece_tables.hpp"
#include "mobility.hpp"
#include "evaluation_features.hpp"
#include "evaluation.hpp"

namespace ac {

int ai_evaluate_absolute(const Position *state) {
    int antFiles[2][Columns];
    int bishopCount[2];
    Square kingPos[2];
    int phase;
    int score;
    int row;
    int col;

    // init
    memset(antFiles, 0, sizeof(antFiles));
    memset(bishopCount, 0, sizeof(bishopCount));
    kingPos[enum_index(Color::White)] = create_position(-1, -1);
    kingPos[enum_index(Color::Black)] = create_position(-1, -1);
    phase = 0;
    score = 0;

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece = get_piece(&state->board, create_position(row, col));

            if (piece.type == PieceType::Empty) {
                continue;
            }

            if (piece.type == PieceType::Ant) {
                ++antFiles[enum_index(piece.color)][col];
            }
            if (piece.type == PieceType::Bishop) {
                ++bishopCount[enum_index(piece.color)];
            }
            if (piece.type == PieceType::King) {
                kingPos[enum_index(piece.color)] = create_position(row, col);
            }

            phase += ai_phase_value(piece.type);
        }
    }

    // robust
    if (phase > AI_MAX_PHASE) {
        phase = AI_MAX_PHASE;
    }

    // ai_piece_value + PST + mobility
    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Square pos = create_position(row, col);
            Piece piece = get_piece(&state->board, pos);
            int value;

            if (piece.type == PieceType::Empty) {
                continue;
            }

            value = ai_piece_value(piece.type);
            value += ai_pst_bonus(piece, row, col, phase);
            if (piece.type == PieceType::Knight || piece.type == PieceType::Anteater) {
                value += ai_piece_mobility(&state->board, pos, piece) * ai_mobility_weight(piece.type);
            }

            if (piece.color == Color::White) {
                score += value;
            } else {
                score -= value;
            }
        }
    }

    // bishop pair
    if (bishopCount[enum_index(Color::White)] >= 2) {
        score += 36;
    }
    if (bishopCount[enum_index(Color::Black)] >= 2) {
        score -= 36;
    }

    score += ai_evaluate_pawns(state, Color::White, antFiles, phase);
    score -= ai_evaluate_pawns(state, Color::Black, antFiles, phase);
    score += ai_evaluate_rook_and_queen_files(state, Color::White, antFiles, phase);
    score -= ai_evaluate_rook_and_queen_files(state, Color::Black, antFiles, phase);
    score += ai_evaluate_king_safety(state, Color::White, kingPos[enum_index(Color::White)], phase);
    score -= ai_evaluate_king_safety(state, Color::Black, kingPos[enum_index(Color::Black)], phase);
    score += ai_evaluate_development(state, Color::White, phase);
    score -= ai_evaluate_development(state, Color::Black, phase);
    score += ai_evaluate_anteater_threats(&state->board, Color::White);
    score -= ai_evaluate_anteater_threats(&state->board, Color::Black);

    // outpost limitation
    if (phase >= 10) {
        score += ai_evaluate_outposts(&state->board, Color::White, phase);
        score -= ai_evaluate_outposts(&state->board, Color::Black, phase);
    }

    return score;
}

int ai_evaluate_relative(const Position *state) {
    int absoluteScore;

    absoluteScore = ai_evaluate_absolute(state);
    // tempo bonus
    if (state->currentTurn == Color::White) {
        return absoluteScore + 1;
    }

    return -absoluteScore + 1;
}

} // namespace ac
