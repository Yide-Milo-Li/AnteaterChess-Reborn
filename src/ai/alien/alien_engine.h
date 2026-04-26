/*
 * Alien plugin — public bridge interface.
 *
 * The host (V_FINAL src/ai/ai.c) probes for this header via __has_include
 * and routes DIFFICULTY_EXPERIMENTAL through here. If the entire
 * src/ai/alien/ folder is removed the include fails, the host falls back
 * to DIFFICULTY_HARD silently, and the build keeps working.
 *
 * This header MUST only reference V_FINAL types (GameState, Move).
 * Alien-internal types (AlienState, AlienMove, ...) live behind this
 * boundary in alien_engine_native.c via a flat C ABI.
 */
#ifndef AI_ALIEN_ALIEN_ENGINE_H
#define AI_ALIEN_ALIEN_ENGINE_H

#include "core/gamestate.h"
#include "core/move.h"

/* Generate a move using the alien engine.
 * Returns 0 on success and writes the chosen move to *move.
 * Returns non-zero on failure (the host should fall back to its own
 * search rather than forfeit). */
int alien_plugin_generate_move(const GameState *state, Move *move, int time_ms);

#endif
