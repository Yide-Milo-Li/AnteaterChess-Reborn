#include "internal.hpp"

namespace ac {
int ai_elapsed_ms(const SearchData *ctx) {
    int64_t now;

    if (ctx == NULL || read_clock(ctx, &now) != Status::Ok || now < ctx->searchStartMs) {
        return 0;
    }
    if (now - ctx->searchStartMs > INT_MAX) {
        return INT_MAX;
    }
    return (int)(now - ctx->searchStartMs);
}

int ai_time_is_up(SearchData *ctx) {
    if (ctx->options.stop.stop_requested()) {
        ctx->stopSearch = 1;
        ctx->failure = Status::Cancelled;
    }
    if ((ctx->nodes & 255) == 0 && ai_elapsed_ms(ctx) >= ctx->timeLimitMs)
        ctx->stopSearch = 1;
    return ctx->stopSearch;
}

Status ai_init_search_context(SearchData *ctx, int timeLimitMs) {
    ctx->nodes = 0;
    ctx->stopSearch = 0;
    ctx->failure = Status::Ok;
    ctx->completedDepth = 0;
    ctx->timeLimitMs = timeLimitMs;
    ctx->softTimeLimitMs = (int)((int64_t)timeLimitMs * 4 / 5);
    if (read_clock(ctx, &ctx->searchStartMs) != Status::Ok)
        return Status::InvalidArgument;
    ++ctx->ttGeneration;
    if (!ctx->ttGeneration)
        ++ctx->ttGeneration;
    ctx->generation = ctx->ttGeneration;
    return Status::Ok;
}

int ai_build_game_hash_history(SearchData *ctx, const Position *state) {
    int n = int(ctx->options.hashes.size());
    if (n > 0)
        memcpy(ctx->gameHashes, ctx->options.hashes.data(), (size_t)n * sizeof(uint64_t));
    else {
        n = 1;
        ctx->gameHashes[0] = state->hash;
    }
    ctx->gameHashCount = n;
    ctx->gameHistoryStart = 0;
    return 0;
}

int ai_node_is_repetition(const SearchData *ctx, uint64_t key, int ply) {
    int index;

    if (ctx == NULL || ctx->nullMoveActive[ply]) {
        return 0;
    }

    // search stack: only go back to last irreversible move
    for (index = ply - 1; index >= ctx->repetitionLimit[ply]; --index) {
        if (ctx->hashStack[index].value == key) {
            return 1;
        }
    }
    // game history: only when no irreversible move in search stack
    if (ctx->repetitionLimit[ply] == 0) {
        for (index = ctx->gameHistoryStart; index + 1 < ctx->gameHashCount; ++index) {
            if (ctx->gameHashes[index] == key) {
                return 1;
            }
        }
    }

    return 0;
}

Status ai_generate_search_moves(Position *state, MoveList *list, int onlyNoisy) {
    Status status = generate_moves(state, list);
    if (status != Status::Ok)
        return status;
    if (onlyNoisy) {
        int n = 0;
        for (int i = 0; i < list->count; ++i)
            if (ai_is_noisy_move(&list->moves[i]))
                list->moves[n++] = list->moves[i];
        list->count = n;
    }
    return Status::Ok;
}

int ai_side_has_major_material(const Board *board, Color color) {
    int row;
    int col;

    for (row = 0; row < Rows; ++row) {
        for (col = 0; col < Columns; ++col) {
            Piece piece = board->cells[row][col];

            if (piece.color != color) {
                continue;
            }
            switch (piece.type) {
            case PieceType::Knight:
            case PieceType::Bishop:
            case PieceType::Rook:
            case PieceType::Queen:
            case PieceType::Anteater:
                return 1;
            case PieceType::Ant:
            case PieceType::King:
            case PieceType::Empty:
            default:
                break;
            }
        }
    }

    return 0;
}

int ai_try_null_move(SearchData *ctx, Position *state, int depth, int beta, int ply) {
    if (ply + 1 >= AI_MAX_PLY)
        return beta - 1;
    Position saved = *state;
    state->currentTurn = state->currentTurn == Color::White ? Color::Black : Color::White;
    state->enPassant = create_position(-1, -1);
    ++state->moveCount;
    state->hash = position_hash(state);
    derive_hash(state, &ctx->hashStack[ply + 1]);
    ctx->repetitionLimit[ply + 1] = ply + 1;
    ctx->nullMoveActive[ply + 1] = 1;
    int score = -ai_alpha_beta(ctx, state, depth - 1 - NULL_MOVE_R - (depth >= 6), -beta, -beta + 1, ply + 1, 0);
    *state = saved;
    return score;
}

int ai_alpha_beta(SearchData *ctx, Position *state, int depth, int alpha, int beta, int ply, int allowNull) {
    TTEntry ttEntry;
    int ttHit;
    int originalAlpha;
    int bestScore;
    int inCheck;
    int index;
    int bestMoveValid;
    int legalCount;
    Move bestMove;
    MoveList *moves;
    uint64_t key;
    int pvNode;
    Color movingSide;

    // time check, return current static eval
    if (ai_time_is_up(ctx)) {
        return ai_evaluate_relative(state);
    }

    // ply too deep, just return eval
    if (ply >= AI_MAX_PLY - 2) {
        return ai_evaluate_relative(state);
    }

    ++ctx->nodes;
    key = ctx->hashStack[ply].value;
    // repetition = draw
    if (ai_node_is_repetition(ctx, key, ply)) {
        return 0;
    }
    // mate distance pruning
    if (alpha < -AI_MATE + ply) {
        alpha = -AI_MATE + ply;
    }
    if (beta > AI_MATE - ply - 1) {
        beta = AI_MATE - ply - 1;
    }
    if (alpha >= beta) {
        return alpha;
    }
    // TT cutoff
    ttHit = ai_tt_lookup(ctx, key, &ttEntry);
    pvNode = (beta - alpha > 1);
    if (ttHit && ttEntry.depth >= depth) {
        int ttScore = ai_score_from_tt(ttEntry.score, ply);

        if (ttEntry.flag == TT_FLAG_EXACT) {
            return ttScore;
        }
        if (!pvNode && ttEntry.flag == TT_FLAG_LOWER && ttScore >= beta) {
            return ttScore;
        }
        if (!pvNode && ttEntry.flag == TT_FLAG_UPPER && ttScore <= alpha) {
            return ttScore;
        }
    }

    inCheck = is_in_check(state, state->currentTurn);
    // check extension: search 1 more ply when in check
    if (inCheck) {
        ++depth;
    }
    // depth 0: drop into ai_quiescence
    if (depth <= 0) {
        return ai_quiescence(ctx, state, alpha, beta, ply, 0);
    }

    // null move pruning
    if (allowNull && !pvNode && !inCheck && depth >= 3 &&
        ai_side_has_major_material(&state->board, state->currentTurn)) {
        int nullScore = ai_try_null_move(ctx, state, depth, beta, ply);

        if (ctx->stopSearch) {
            return alpha;
        }
        if (nullScore >= beta) {
            if (nullScore >= AI_MATE - 1000) {
                nullScore = beta;
            }
            return nullScore;
        }
    }

    moves = &ctx->moveBuffers[ply];
    if (ai_generate_search_moves(state, moves, 0) != Status::Ok) {
        ctx->failure = moves->status;
        ctx->stopSearch = 1;
        return ai_evaluate_relative(state);
    }
    // no legal move: checkmate or stalemate
    if (moves->count == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return 0;
    }

    ai_sort_moves(ctx, state, moves, ply, ttHit ? &ttEntry : NULL);
    originalAlpha = alpha;
    bestScore = -AI_INF;
    bestMoveValid = 0;
    legalCount = 0;
    movingSide = state->currentTurn;

    for (index = 0; index < moves->count; ++index) {
        Move move = moves->moves[index];
        int score;
        int childDepth;
        int reduction;
        int extension;

        if (position_make(state, move, &ctx->undoStack[ply + 1]) != Status::Ok) {
            continue;
        }
        // skip illegal move (leaving own king in check)
        if (is_in_check(state, movingSide) != 0) {
            if (position_unmake(state, &ctx->undoStack[ply + 1]) != Status::Ok) {
                return ai_evaluate_relative(state);
            }
            continue;
        }

        ++legalCount;
        if (derive_hash(state, &ctx->hashStack[ply + 1]) != 0) {
            if (position_unmake(state, &ctx->undoStack[ply + 1]) != Status::Ok) {
                return ai_evaluate_relative(state);
            }
            continue;
        }
        // irreversible move resets the repetition window
        ctx->repetitionLimit[ply + 1] = ai_is_irreversible_move(&move) ? (ply + 1) : ctx->repetitionLimit[ply];
        ctx->nullMoveActive[ply + 1] = ctx->nullMoveActive[ply];

        // tactical extension: promotion / multi-ant anteater
        extension = 0;
        if (depth <= 6) {
            if (ai_is_promotion_move(&move)) {
                extension = 1;
            } else if (move.specialType == SpecialMove::AnteaterCapture && move.captureCount >= 2) {
                extension = 1;
            }
        }
        childDepth = depth - 1 + extension;

        // late move reduction: bigger cut for late, deep, quiet moves
        reduction = 0;
        if (legalCount >= 4 && depth >= 4 && !inCheck && ai_is_quiet_move(&move) && !pvNode) {
            reduction = 1;
            if (legalCount >= 8 && depth >= 5) {
                ++reduction;
            }
            if (legalCount >= 12 && depth >= 8) {
                ++reduction;
            }
            if (reduction > childDepth - 1) {
                reduction = childDepth - 1;
            }
            if (reduction < 0) {
                reduction = 0;
            }
        }

        // PVS: first move full window, others null window then full if needed
        if (legalCount == 1) {
            score = -ai_alpha_beta(ctx, state, childDepth, -beta, -alpha, ply + 1, 1);
        } else {
            int reducedDepth = childDepth - reduction;

            if (reducedDepth < 0) {
                reducedDepth = 0;
            }
            // null window with reduction
            score = -ai_alpha_beta(ctx, state, reducedDepth, -alpha - 1, -alpha, ply + 1, 1);
            // re-search at full depth if reduction was wrong
            if (!ctx->stopSearch && reduction > 0 && score > alpha) {
                score = -ai_alpha_beta(ctx, state, childDepth, -alpha - 1, -alpha, ply + 1, 1);
            }
            // re-search with full window when score raised alpha
            if (!ctx->stopSearch && score > alpha && score < beta) {
                score = -ai_alpha_beta(ctx, state, childDepth, -beta, -alpha, ply + 1, 1);
            }
        }

        if (position_unmake(state, &ctx->undoStack[ply + 1]) != Status::Ok) {
            return ai_evaluate_relative(state);
        }
        if (ctx->stopSearch) {
            return alpha;
        }

        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
            bestMoveValid = 1;
        }
        if (score > alpha) {
            alpha = score;
        }
        // beta cutoff
        if (alpha >= beta) {
            if (ai_is_quiet_move(&move)) {
                ai_save_killer(ctx, ply, move);
                ai_update_history_score(ctx, movingSide, move, depth * depth + 8);
            }
            ai_tt_store(ctx, key, depth, ply, alpha, TT_FLAG_LOWER, &move, ctx->generation);
            return alpha;
        }
        // small penalty for quiet move that did not raise alpha
        if (ai_is_quiet_move(&move)) {
            ai_update_history_score(ctx, movingSide, move, -(depth + 1));
        }
    }

    if (legalCount == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return 0;
    }
    if (!bestMoveValid) {
        return (inCheck != 0) ? (-AI_MATE + ply) : 0;
    }

    ai_tt_store(ctx, key, depth, ply, bestScore, (bestScore <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT,
                &bestMove, ctx->generation);
    return bestScore;
}

int ai_quiescence(SearchData *ctx, Position *state, int alpha, int beta, int ply, int qDepth) {
    TTEntry ttEntry;
    int ttHit;
    int originalAlpha;
    int standPat;
    int inCheck;
    int index;
    int legalCount;
    MoveList *moves;
    uint64_t key;
    Color movingSide;

    if (ai_time_is_up(ctx)) {
        return ai_evaluate_relative(state);
    }
    if (ply >= AI_MAX_PLY - 2) {
        return ai_evaluate_relative(state);
    }

    ++ctx->nodes;
    originalAlpha = alpha;
    key = ctx->hashStack[ply].value;
    if (ai_node_is_repetition(ctx, key, ply)) {
        return 0;
    }
    ttHit = ai_tt_lookup(ctx, key, &ttEntry);
    if (ttHit && ttEntry.depth >= 0) {
        int ttScore = ai_score_from_tt(ttEntry.score, ply);

        if (ttEntry.flag == TT_FLAG_EXACT) {
            return ttScore;
        }
        if (ttEntry.flag == TT_FLAG_LOWER && ttScore >= beta) {
            return ttScore;
        }
        if (ttEntry.flag == TT_FLAG_UPPER && ttScore <= alpha) {
            return ttScore;
        }
    }

    inCheck = is_in_check(state, state->currentTurn);
    standPat = alpha;
    // stand-pat: assume not moving is OK if not in check
    if (!inCheck) {
        standPat = ai_evaluate_relative(state);
        if (standPat >= beta) {
            ai_tt_store(ctx, key, 0, ply, standPat, TT_FLAG_LOWER, NULL, ctx->generation);
            return standPat;
        }
        if (standPat > alpha) {
            alpha = standPat;
        }
        // q depth limit reached, return current
        if (qDepth >= AI_Q_DEPTH) {
            ai_tt_store(ctx, key, 0, ply, alpha, (alpha <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, NULL,
                        ctx->generation);
            return alpha;
        }
    } else if (qDepth >= AI_Q_DEPTH + 2) {
        // when in check we go a bit deeper but still cap it
        return ai_evaluate_relative(state);
    }

    moves = &ctx->moveBuffers[ply];
    if (ai_generate_search_moves(state, moves, !inCheck) != Status::Ok) {
        ctx->failure = moves->status;
        ctx->stopSearch = 1;
        return alpha;
    }
    if (moves->count == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return alpha;
    }

    ai_sort_moves(ctx, state, moves, ply, ttHit ? &ttEntry : NULL);
    legalCount = 0;
    movingSide = state->currentTurn;
    for (index = 0; index < moves->count; ++index) {
        Move move = moves->moves[index];
        int score;

        if (!inCheck) {
            int quickMargin;

            // skip clearly losing capture using SEE
            quickMargin = ai_quick_exchange_margin(&move);
            if (quickMargin < 0 && !ai_is_promotion_move(&move)) {
                int seeScore = ai_see_move_score(state, &move);

                if (seeScore < 0) {
                    continue;
                }
                quickMargin = seeScore;
            }
            // delta pruning: skip if even best capture cannot raise alpha
            if (standPat + ai_see_initial_gain(&move) + 100 < alpha && !ai_is_promotion_move(&move)) {
                continue;
            }
            (void)quickMargin;
        }

        if (position_make(state, move, &ctx->undoStack[ply + 1]) != Status::Ok) {
            continue;
        }
        if (is_in_check(state, movingSide) != 0) {
            if (position_unmake(state, &ctx->undoStack[ply + 1]) != Status::Ok) {
                return alpha;
            }
            continue;
        }

        ++legalCount;
        if (derive_hash(state, &ctx->hashStack[ply + 1]) != 0) {
            if (position_unmake(state, &ctx->undoStack[ply + 1]) != Status::Ok) {
                return alpha;
            }
            continue;
        }
        ctx->repetitionLimit[ply + 1] = ai_is_irreversible_move(&move) ? (ply + 1) : ctx->repetitionLimit[ply];
        ctx->nullMoveActive[ply + 1] = ctx->nullMoveActive[ply];

        score = -ai_quiescence(ctx, state, -beta, -alpha, ply + 1, qDepth + 1);
        if (position_unmake(state, &ctx->undoStack[ply + 1]) != Status::Ok) {
            return alpha;
        }
        if (ctx->stopSearch) {
            return alpha;
        }
        if (score >= beta) {
            ai_tt_store(ctx, key, 0, ply, score, TT_FLAG_LOWER, &move, ctx->generation);
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (legalCount == 0 && inCheck) {
        return -AI_MATE + ply;
    }

    ai_tt_store(ctx, key, 0, ply, alpha, (alpha <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, NULL,
                ctx->generation);
    return alpha;
}

int ai_search_best_move(SearchData *ctx, const Position *state, int maxDepth, int maxTimeMs, Move *bestMove) {
    Position searchState;
    MoveList *rootMoves;
    TTEntry rootEntry;
    int rootScores[MaxMoves];
    Move currentBest;
    int currentBestScore;
    int depth;

    if (state == NULL || bestMove == NULL) {
        return 1;
    }

    if (maxDepth > AI_MAX_PLY - 2) {
        maxDepth = AI_MAX_PLY - 2;
    }
    if (ai_init_search_context(ctx, maxTimeMs) != Status::Ok) {
        return 1;
    }
    ai_age_history_scores(ctx);

    searchState = *state;
    if (derive_hash(&searchState, &ctx->hashStack[0]) != 0) {

        return 1;
    }
    if (ai_build_game_hash_history(ctx, &searchState) != 0) {
        ctx->gameHashes[0] = ctx->hashStack[0].value;
        ctx->gameHashCount = 1;
        ctx->gameHistoryStart = 0;
    }
    ctx->repetitionLimit[0] = 0;
    ctx->nullMoveActive[0] = 0;

    rootMoves = &ctx->moveBuffers[0];
    if (generate_legal_moves(&searchState, rootMoves) != Status::Ok || rootMoves->count <= 0) {
        ctx->failure = rootMoves->status;

        return 1;
    }

    // initial sort, prefer TT best move if any
    memset(rootScores, 0, sizeof(rootScores));
    if (ai_tt_lookup(ctx, ctx->hashStack[0].value, &rootEntry)) {
        ai_sort_moves(ctx, &searchState, rootMoves, 0, &rootEntry);
    } else {
        ai_sort_moves(ctx, &searchState, rootMoves, 0, NULL);
    }

    *bestMove = rootMoves->moves[0];
    // only one move, no search needed
    if (rootMoves->count == 1) {

        return 0;
    }

    currentBest = rootMoves->moves[0];
    currentBestScore = -AI_INF;

    // iterative deepening
    for (depth = 1; depth <= maxDepth; ++depth) {
        Move iterationBest;
        int iterationBestScore;
        int aspiration;
        int alphaBase;
        int betaBase;
        int attempt;

        iterationBest = currentBest;
        iterationBestScore = -AI_INF;
        // aspiration window: search narrow window first, widen on fail
        aspiration = ASPIRATION_WINDOW;
        if (depth > 1 && currentBestScore > -AI_INF / 2) {
            alphaBase = currentBestScore - aspiration;
            betaBase = currentBestScore + aspiration;
        } else {
            alphaBase = -AI_INF;
            betaBase = AI_INF;
        }

        // up to 4 widening attempts
        for (attempt = 0; attempt < 4; ++attempt) {
            int alpha;
            int beta;
            int localAlpha;
            int index;

            alpha = alphaBase;
            beta = betaBase;
            localAlpha = alpha;
            iterationBest = currentBest;
            iterationBestScore = -AI_INF;

            for (index = 0; index < rootMoves->count; ++index) {
                Move move = rootMoves->moves[index];
                int score;

                if (position_make(&searchState, move, &ctx->undoStack[1]) != Status::Ok) {
                    rootScores[index] = -AI_INF;
                    continue;
                }
                if (derive_hash(&searchState, &ctx->hashStack[1]) != 0) {
                    if (position_unmake(&searchState, &ctx->undoStack[1]) != Status::Ok) {

                        return 1;
                    }
                    rootScores[index] = -AI_INF;
                    continue;
                }
                ctx->repetitionLimit[1] = ai_is_irreversible_move(&move) ? 1 : 0;
                ctx->nullMoveActive[1] = 0;

                // PVS at root: first move full window, others null then full re-search
                if (index == 0) {
                    score = -ai_alpha_beta(ctx, &searchState, depth - 1, -beta, -localAlpha, 1, 1);
                } else {
                    score = -ai_alpha_beta(ctx, &searchState, depth - 1, -localAlpha - 1, -localAlpha, 1, 1);
                    if (!ctx->stopSearch && score > localAlpha && score < beta) {
                        score = -ai_alpha_beta(ctx, &searchState, depth - 1, -beta, -localAlpha, 1, 1);
                    }
                }

                if (position_unmake(&searchState, &ctx->undoStack[1]) != Status::Ok) {

                    return 1;
                }
                if (ctx->stopSearch) {
                    break;
                }

                rootScores[index] = score;
                if (score > iterationBestScore) {
                    iterationBestScore = score;
                    iterationBest = move;
                }
                if (score > localAlpha) {
                    localAlpha = score;
                }
            }

            if (ctx->stopSearch) {
                break;
            }

            // failed low/high: widen the failing side and retry
            if (alphaBase != -AI_INF || betaBase != AI_INF) {
                if (iterationBestScore <= alpha) {
                    alphaBase -= aspiration * 4;
                    if (alphaBase < -AI_INF) {
                        alphaBase = -AI_INF;
                    }
                    aspiration *= 2;
                    continue;
                }
                if (iterationBestScore >= beta) {
                    betaBase += aspiration * 4;
                    if (betaBase > AI_INF) {
                        betaBase = AI_INF;
                    }
                    aspiration *= 2;
                    continue;
                }
            }
            break;
        }

        if (ctx->stopSearch) {
            break;
        }

        ctx->completedDepth = depth;
        currentBest = iterationBest;
        currentBestScore = iterationBestScore;
        *bestMove = currentBest;
        // resort root moves so good ones go first next iteration
        ai_sort_root_moves_by_scores(rootMoves, rootScores);
        ai_tt_store(ctx, ctx->hashStack[0].value, depth, 0, currentBestScore, TT_FLAG_EXACT, &currentBest,
                    ctx->generation);

        // mate found, stop early
        if (currentBestScore >= AI_MATE - 1000 || currentBestScore <= -AI_MATE + 1000) {
            break;
        }
        // soft time limit: don't start a new iteration we won't finish
        if (ctx->softTimeLimitMs > 0 && ai_elapsed_ms(ctx) >= ctx->softTimeLimitMs) {
            break;
        }
    }

    return 0;
}

} // namespace ac
