#include "internal.h"
int ac_ai_elapsed_ms(const AcSearchContext *ctx) {
    int64_t now;

    if (ctx == NULL || read_clock(ctx, &now) != 0 || now < ctx->searchStartMs) {
        return 0;
    }
    if (now - ctx->searchStartMs > INT_MAX) {
        return INT_MAX;
    }
    return (int)(now - ctx->searchStartMs);
}

int ac_ai_time_is_up(AcSearchContext *ctx) {
    if (ctx->options.cancelled && ctx->options.cancelled(ctx->options.cancelContext)) {
        ctx->stopSearch = 1;
        ctx->failure = AC_CANCELLED;
    }
    if ((ctx->nodes & 255) == 0 && ac_ai_elapsed_ms(ctx) >= ctx->timeLimitMs)
        ctx->stopSearch = 1;
    return ctx->stopSearch;
}

int ac_ai_init_search_context(AcSearchContext *ctx, int timeLimitMs) {
    ctx->nodes = 0;
    ctx->stopSearch = 0;
    ctx->failure = AC_OK;
    ctx->completedDepth = 0;
    ctx->timeLimitMs = timeLimitMs;
    ctx->softTimeLimitMs = (int)((int64_t)timeLimitMs * 4 / 5);
    if (read_clock(ctx, &ctx->searchStartMs))
        return AC_INVALID_ARGUMENT;
    ++ctx->ttGeneration;
    if (!ctx->ttGeneration)
        ++ctx->ttGeneration;
    ctx->generation = ctx->ttGeneration;
    return AC_OK;
}

int ac_ai_build_game_hash_history(AcSearchContext *ctx, const AcPosition *state) {
    int n = ctx->options.hashCount;
    if (n > 0 && ctx->options.hashes)
        memcpy(ctx->gameHashes, ctx->options.hashes, (size_t)n * sizeof(uint64_t));
    else {
        n = 1;
        ctx->gameHashes[0] = state->hash;
    }
    ctx->gameHashCount = n;
    ctx->gameHistoryStart = 0;
    return 0;
}

int ac_ai_node_is_repetition(const AcSearchContext *ctx, uint64_t key, int ply) {
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

int ac_ai_generate_search_moves(AcPosition *state, AcMoveList *list, int onlyNoisy) {
    int status = ac_generate_moves(state, list);
    if (status)
        return status;
    if (onlyNoisy) {
        int n = 0;
        for (int i = 0; i < list->count; ++i)
            if (ac_ai_is_noisy_move(&list->moves[i]))
                list->moves[n++] = list->moves[i];
        list->count = n;
    }
    return 0;
}

int ac_ai_side_has_major_material(const AcBoard *board, AcColor color) {
    int row;
    int col;

    for (row = 0; row < AC_ROWS; ++row) {
        for (col = 0; col < AC_COLS; ++col) {
            AcPiece piece = board->cells[row][col];

            if (piece.color != color) {
                continue;
            }
            switch (piece.type) {
            case AC_KNIGHT:
            case AC_BISHOP:
            case AC_ROOK:
            case AC_QUEEN:
            case AC_ANTEATER:
                return 1;
            case AC_ANT:
            case AC_KING:
            case AC_EMPTY_PIECE:
            default:
                break;
            }
        }
    }

    return 0;
}

int ac_ai_try_null_move(AcSearchContext *ctx, AcPosition *state, int depth, int beta, int ply) {
    if (ply + 1 >= AI_MAX_PLY)
        return beta - 1;
    AcPosition saved = *state;
    state->currentTurn = state->currentTurn == AC_WHITE ? AC_BLACK : AC_WHITE;
    state->enPassant = ac_create_position(-1, -1);
    ++state->moveCount;
    state->hash = ac_position_hash(state);
    derive_hash(state, &ctx->hashStack[ply + 1]);
    ctx->repetitionLimit[ply + 1] = ply + 1;
    ctx->nullMoveActive[ply + 1] = 1;
    int score = -ac_ai_alpha_beta(ctx, state, depth - 1 - NULL_MOVE_R - (depth >= 6), -beta, -beta + 1, ply + 1, 0);
    *state = saved;
    return score;
}

int ac_ai_alpha_beta(AcSearchContext *ctx, AcPosition *state, int depth, int alpha, int beta, int ply, int allowNull) {
    TTEntry ttEntry;
    int ttHit;
    int originalAlpha;
    int bestScore;
    int inCheck;
    int index;
    int bestMoveValid;
    int legalCount;
    AcMove bestMove;
    AcMoveList *moves;
    uint64_t key;
    int pvNode;
    AcColor movingSide;

    // time check, return current static eval
    if (ac_ai_time_is_up(ctx)) {
        return ac_ai_evaluate_relative(state);
    }

    // ply too deep, just return eval
    if (ply >= AI_MAX_PLY - 2) {
        return ac_ai_evaluate_relative(state);
    }

    ++ctx->nodes;
    key = ctx->hashStack[ply].value;
    // repetition = draw
    if (ac_ai_node_is_repetition(ctx, key, ply)) {
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
    ttHit = ac_ai_tt_lookup(ctx, key, &ttEntry);
    pvNode = (beta - alpha > 1);
    if (ttHit && ttEntry.depth >= depth) {
        int ttScore = ac_ai_score_from_tt(ttEntry.score, ply);

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

    inCheck = ac_is_in_check(state, state->currentTurn);
    // check extension: search 1 more ply when in check
    if (inCheck) {
        ++depth;
    }
    // depth 0: drop into ac_ai_quiescence
    if (depth <= 0) {
        return ac_ai_quiescence(ctx, state, alpha, beta, ply, 0);
    }

    // null move pruning
    if (allowNull && !pvNode && !inCheck && depth >= 3 &&
        ac_ai_side_has_major_material(&state->board, state->currentTurn)) {
        int nullScore = ac_ai_try_null_move(ctx, state, depth, beta, ply);

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
    if (ac_ai_generate_search_moves(state, moves, 0) != 0) {
        ctx->failure = moves->status;
        ctx->stopSearch = 1;
        return ac_ai_evaluate_relative(state);
    }
    // no legal move: checkmate or stalemate
    if (moves->count == 0) {
        if (inCheck) {
            return -AI_MATE + ply;
        }
        return 0;
    }

    ac_ai_sort_moves(ctx, state, moves, ply, ttHit ? &ttEntry : NULL);
    originalAlpha = alpha;
    bestScore = -AI_INF;
    bestMoveValid = 0;
    legalCount = 0;
    movingSide = state->currentTurn;

    for (index = 0; index < moves->count; ++index) {
        AcMove move = moves->moves[index];
        int score;
        int childDepth;
        int reduction;
        int extension;

        if (ac_position_make(state, move, &ctx->undoStack[ply + 1]) != 0) {
            continue;
        }
        // skip illegal move (leaving own king in check)
        if (ac_is_in_check(state, movingSide) != 0) {
            if (ac_position_unmake(state, &ctx->undoStack[ply + 1]) != 0) {
                return ac_ai_evaluate_relative(state);
            }
            continue;
        }

        ++legalCount;
        if (derive_hash(state, &ctx->hashStack[ply + 1]) != 0) {
            if (ac_position_unmake(state, &ctx->undoStack[ply + 1]) != 0) {
                return ac_ai_evaluate_relative(state);
            }
            continue;
        }
        // irreversible move resets the repetition window
        ctx->repetitionLimit[ply + 1] = ac_ai_is_irreversible_move(&move) ? (ply + 1) : ctx->repetitionLimit[ply];
        ctx->nullMoveActive[ply + 1] = ctx->nullMoveActive[ply];

        // tactical extension: promotion / multi-ant anteater
        extension = 0;
        if (depth <= 6) {
            if (ac_ai_is_promotion_move(&move)) {
                extension = 1;
            } else if (move.specialType == AC_ANTEATER_CAPTURE && move.captureCount >= 2) {
                extension = 1;
            }
        }
        childDepth = depth - 1 + extension;

        // late move reduction: bigger cut for late, deep, quiet moves
        reduction = 0;
        if (legalCount >= 4 && depth >= 4 && !inCheck && ac_ai_is_quiet_move(&move) && !pvNode) {
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
            score = -ac_ai_alpha_beta(ctx, state, childDepth, -beta, -alpha, ply + 1, 1);
        } else {
            int reducedDepth = childDepth - reduction;

            if (reducedDepth < 0) {
                reducedDepth = 0;
            }
            // null window with reduction
            score = -ac_ai_alpha_beta(ctx, state, reducedDepth, -alpha - 1, -alpha, ply + 1, 1);
            // re-search at full depth if reduction was wrong
            if (!ctx->stopSearch && reduction > 0 && score > alpha) {
                score = -ac_ai_alpha_beta(ctx, state, childDepth, -alpha - 1, -alpha, ply + 1, 1);
            }
            // re-search with full window when score raised alpha
            if (!ctx->stopSearch && score > alpha && score < beta) {
                score = -ac_ai_alpha_beta(ctx, state, childDepth, -beta, -alpha, ply + 1, 1);
            }
        }

        if (ac_position_unmake(state, &ctx->undoStack[ply + 1]) != 0) {
            return ac_ai_evaluate_relative(state);
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
            if (ac_ai_is_quiet_move(&move)) {
                ac_ai_save_killer(ctx, ply, move);
                ac_ai_update_history_score(ctx, movingSide, move, depth * depth + 8);
            }
            ac_ai_tt_store(ctx, key, depth, ply, alpha, TT_FLAG_LOWER, &move, ctx->generation);
            return alpha;
        }
        // small penalty for quiet move that did not raise alpha
        if (ac_ai_is_quiet_move(&move)) {
            ac_ai_update_history_score(ctx, movingSide, move, -(depth + 1));
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

    ac_ai_tt_store(ctx, key, depth, ply, bestScore, (bestScore <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT,
                   &bestMove, ctx->generation);
    return bestScore;
}

int ac_ai_quiescence(AcSearchContext *ctx, AcPosition *state, int alpha, int beta, int ply, int qDepth) {
    TTEntry ttEntry;
    int ttHit;
    int originalAlpha;
    int standPat;
    int inCheck;
    int index;
    int legalCount;
    AcMoveList *moves;
    uint64_t key;
    AcColor movingSide;

    if (ac_ai_time_is_up(ctx)) {
        return ac_ai_evaluate_relative(state);
    }
    if (ply >= AI_MAX_PLY - 2) {
        return ac_ai_evaluate_relative(state);
    }

    ++ctx->nodes;
    originalAlpha = alpha;
    key = ctx->hashStack[ply].value;
    if (ac_ai_node_is_repetition(ctx, key, ply)) {
        return 0;
    }
    ttHit = ac_ai_tt_lookup(ctx, key, &ttEntry);
    if (ttHit && ttEntry.depth >= 0) {
        int ttScore = ac_ai_score_from_tt(ttEntry.score, ply);

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

    inCheck = ac_is_in_check(state, state->currentTurn);
    standPat = alpha;
    // stand-pat: assume not moving is OK if not in check
    if (!inCheck) {
        standPat = ac_ai_evaluate_relative(state);
        if (standPat >= beta) {
            ac_ai_tt_store(ctx, key, 0, ply, standPat, TT_FLAG_LOWER, NULL, ctx->generation);
            return standPat;
        }
        if (standPat > alpha) {
            alpha = standPat;
        }
        // q depth limit reached, return current
        if (qDepth >= AI_Q_DEPTH) {
            ac_ai_tt_store(ctx, key, 0, ply, alpha, (alpha <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, NULL,
                           ctx->generation);
            return alpha;
        }
    } else if (qDepth >= AI_Q_DEPTH + 2) {
        // when in check we go a bit deeper but still cap it
        return ac_ai_evaluate_relative(state);
    }

    moves = &ctx->moveBuffers[ply];
    if (ac_ai_generate_search_moves(state, moves, !inCheck) != 0) {
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

    ac_ai_sort_moves(ctx, state, moves, ply, ttHit ? &ttEntry : NULL);
    legalCount = 0;
    movingSide = state->currentTurn;
    for (index = 0; index < moves->count; ++index) {
        AcMove move = moves->moves[index];
        int score;

        if (!inCheck) {
            int quickMargin;

            // skip clearly losing capture using SEE
            quickMargin = ac_ai_quick_exchange_margin(&move);
            if (quickMargin < 0 && !ac_ai_is_promotion_move(&move)) {
                int seeScore = ac_ai_see_move_score(state, &move);

                if (seeScore < 0) {
                    continue;
                }
                quickMargin = seeScore;
            }
            // delta pruning: skip if even best capture cannot raise alpha
            if (standPat + ac_ai_see_initial_gain(&move) + 100 < alpha && !ac_ai_is_promotion_move(&move)) {
                continue;
            }
            (void)quickMargin;
        }

        if (ac_position_make(state, move, &ctx->undoStack[ply + 1]) != 0) {
            continue;
        }
        if (ac_is_in_check(state, movingSide) != 0) {
            if (ac_position_unmake(state, &ctx->undoStack[ply + 1]) != 0) {
                return alpha;
            }
            continue;
        }

        ++legalCount;
        if (derive_hash(state, &ctx->hashStack[ply + 1]) != 0) {
            if (ac_position_unmake(state, &ctx->undoStack[ply + 1]) != 0) {
                return alpha;
            }
            continue;
        }
        ctx->repetitionLimit[ply + 1] = ac_ai_is_irreversible_move(&move) ? (ply + 1) : ctx->repetitionLimit[ply];
        ctx->nullMoveActive[ply + 1] = ctx->nullMoveActive[ply];

        score = -ac_ai_quiescence(ctx, state, -beta, -alpha, ply + 1, qDepth + 1);
        if (ac_position_unmake(state, &ctx->undoStack[ply + 1]) != 0) {
            return alpha;
        }
        if (ctx->stopSearch) {
            return alpha;
        }
        if (score >= beta) {
            ac_ai_tt_store(ctx, key, 0, ply, score, TT_FLAG_LOWER, &move, ctx->generation);
            return score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    if (legalCount == 0 && inCheck) {
        return -AI_MATE + ply;
    }

    ac_ai_tt_store(ctx, key, 0, ply, alpha, (alpha <= originalAlpha) ? TT_FLAG_UPPER : TT_FLAG_EXACT, NULL,
                   ctx->generation);
    return alpha;
}

int ac_search_depth(AcAIDifficulty difficulty) {
    switch (difficulty) {
    case AC_DIFFICULTY_EASY:
        return 2;
    case AC_DIFFICULTY_MEDIUM:
        return 10;
    case AC_DIFFICULTY_HARD:
    case AC_DIFFICULTY_EXPERIMENTAL:
    case AC_DIFFICULTY_TOURNAMENT:
        return 24;
    case AC_DIFFICULTY_NONE:
    default:
        return 8;
    }
}

int ac_ai_search_best_move(AcSearchContext *ctx, const AcPosition *state, int maxDepth, int maxTimeMs,
                           AcMove *bestMove) {
    AcPosition searchState;
    AcMoveList *rootMoves;
    TTEntry rootEntry;
    int rootScores[AC_MAX_MOVES];
    AcMove currentBest;
    int currentBestScore;
    int depth;

    if (state == NULL || bestMove == NULL) {
        return 1;
    }

    if (maxDepth > AI_MAX_PLY - 2) {
        maxDepth = AI_MAX_PLY - 2;
    }
    if (ac_ai_init_search_context(ctx, maxTimeMs) != 0) {
        return 1;
    }
    ac_ai_age_history_scores(ctx);

    searchState = *state;
    if (derive_hash(&searchState, &ctx->hashStack[0]) != 0) {

        return 1;
    }
    if (ac_ai_build_game_hash_history(ctx, &searchState) != 0) {
        ctx->gameHashes[0] = ctx->hashStack[0].value;
        ctx->gameHashCount = 1;
        ctx->gameHistoryStart = 0;
    }
    ctx->repetitionLimit[0] = 0;
    ctx->nullMoveActive[0] = 0;

    rootMoves = &ctx->moveBuffers[0];
    if (ac_generate_legal_moves(&searchState, rootMoves) != 0 || rootMoves->count <= 0) {
        ctx->failure = rootMoves->status;

        return 1;
    }

    // initial sort, prefer TT best move if any
    memset(rootScores, 0, sizeof(rootScores));
    if (ac_ai_tt_lookup(ctx, ctx->hashStack[0].value, &rootEntry)) {
        ac_ai_sort_moves(ctx, &searchState, rootMoves, 0, &rootEntry);
    } else {
        ac_ai_sort_moves(ctx, &searchState, rootMoves, 0, NULL);
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
        AcMove iterationBest;
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
                AcMove move = rootMoves->moves[index];
                int score;

                if (ac_position_make(&searchState, move, &ctx->undoStack[1]) != 0) {
                    rootScores[index] = -AI_INF;
                    continue;
                }
                if (derive_hash(&searchState, &ctx->hashStack[1]) != 0) {
                    if (ac_position_unmake(&searchState, &ctx->undoStack[1]) != 0) {

                        return 1;
                    }
                    rootScores[index] = -AI_INF;
                    continue;
                }
                ctx->repetitionLimit[1] = ac_ai_is_irreversible_move(&move) ? 1 : 0;
                ctx->nullMoveActive[1] = 0;

                // PVS at root: first move full window, others null then full re-search
                if (index == 0) {
                    score = -ac_ai_alpha_beta(ctx, &searchState, depth - 1, -beta, -localAlpha, 1, 1);
                } else {
                    score = -ac_ai_alpha_beta(ctx, &searchState, depth - 1, -localAlpha - 1, -localAlpha, 1, 1);
                    if (!ctx->stopSearch && score > localAlpha && score < beta) {
                        score = -ac_ai_alpha_beta(ctx, &searchState, depth - 1, -beta, -localAlpha, 1, 1);
                    }
                }

                if (ac_position_unmake(&searchState, &ctx->undoStack[1]) != 0) {

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
        ac_ai_sort_root_moves_by_scores(rootMoves, rootScores);
        ac_ai_tt_store(ctx, ctx->hashStack[0].value, depth, 0, currentBestScore, TT_FLAG_EXACT, &currentBest,
                       ctx->generation);

        // mate found, stop early
        if (currentBestScore >= AI_MATE - 1000 || currentBestScore <= -AI_MATE + 1000) {
            break;
        }
        // soft time limit: don't start a new iteration we won't finish
        if (ctx->softTimeLimitMs > 0 && ac_ai_elapsed_ms(ctx) >= ctx->softTimeLimitMs) {
            break;
        }
    }

    return 0;
}
