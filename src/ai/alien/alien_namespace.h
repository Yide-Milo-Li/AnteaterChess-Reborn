/*
 * Internal-only header. Included BEFORE any alien_chess.h / alien_book.h
 * inside this plugin folder. Renames alien's typedef tokens so they do
 * not collide with the V_FINAL host's identically-named typedefs (Move,
 * State, MoveList, etc.) when both worlds appear in the same TU.
 *
 * Linker-symbol clashes are NOT a concern: alien's source uses snake_case
 * (make_move, gen_legal_moves, init_zobrist, ...) while V_FINAL uses
 * camelCase (applyMove, generateLegalMoves, initZobrist, ...). They never
 * resolve to the same symbol.
 *
 * This file is private to src/ai/alien/. Nothing outside the plugin
 * should include it.
 */
#ifndef ALIEN_NAMESPACE_H
#define ALIEN_NAMESPACE_H

#define Move           AlienMove
#define MoveList       AlienMoveList
#define State          AlienState
#define HistoryEntry   AlienHistoryEntry
#define TTEntry        AlienTTEntry
#define BookEntry      AlienBookEntry
#define Book           AlienBook

#endif
