#ifndef CHESS_AI_SEARCH_INTERNAL_H
#define CHESS_AI_SEARCH_INTERNAL_H

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "core/movelist.h"

typedef int (*AISearchEvaluateAdjustment)(const GameState *state);
typedef int (*AISearchSoftLimit)(const GameState *state,
                                 const MoveList *rootMoves,
                                 int maxTimeMs);
typedef int (*AISearchAllowNullMove)(const GameState *state, int depth);
typedef int (*AISearchExtendMove)(const GameState *stateAfterMove,
                                  const Move *move,
                                  int depth,
                                  int givesCheck);
typedef int (*AISearchStopAfterDepth)(int elapsedMs,
                                      int timeLimitMs,
                                      int softTimeLimitMs,
                                      int rootScoreGap,
                                      int stableDepths);

typedef struct {
    int maxDepth;
    int softNumerator;
    int softDenominator;
    int lmrQuietStart;
    int lmrDepthStart;
    int lmrSecondStart;
    int lmrThirdStart;
    int nullDepthStart;
    int nullReductionBase;
    int nullReductionDeep;
    int tacticalExtensionMaxDepth;
    AISearchEvaluateAdjustment evaluateAdjustment;
    AISearchSoftLimit softLimitMs;
    AISearchAllowNullMove allowNullMove;
    AISearchExtendMove extendMove;
    AISearchStopAfterDepth shouldStopAfterDepth;
} AISearchProfile;

AISearchProfile aiSearchProfileForDifficulty(AIDifficulty difficulty);
int aiSearchBestMoveWithProfile(const GameState *state,
                                const AISearchProfile *profile,
                                int maxTimeMs,
                                Move *bestMove);

#endif
