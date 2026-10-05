#ifndef ANTEATER_RULES_H
#define ANTEATER_RULES_H
#include "anteater/types.hpp"
#include <memory_resource>

namespace ac {
void position_init(Position *position);
uint64_t position_hash(const Position *position);
Status position_apply(Position *position, Move move, Undo *undo,
                      std::pmr::memory_resource *resource = std::pmr::get_default_resource());
Status position_unmake(Position *position, const Undo *undo);
Status position_result(const Position *position, GameResult *result,
                       std::pmr::memory_resource *resource = std::pmr::get_default_resource());

Status generate_moves(const Position *state, MoveList *list);
Status generate_legal_moves(const Position *state, MoveList *list);
Status generate_legal_moves_for_position(const Position *state, Square from, MoveList *list);

Status resolve_move_request(const Position *state, MoveRequest request, Move *resolvedMove,
                            std::pmr::memory_resource *resource = std::pmr::get_default_resource());

enum class SelectionResult { Valid, Empty, OpponentPiece, OutOfBounds };

int validate_move(const Position *state, Move move,
                  std::pmr::memory_resource *resource = std::pmr::get_default_resource());
SelectionResult validate_selection(const Position *state, Square pos);

/* Parse two coordinate text fields directly into a frontend MoveRequest. */
Status parse_move_request_fields(const char *fromText, const char *toText, PromotionChoice promotion,
                                 MoveRequest *request);

void init_move_list(MoveList *list);
Status add_move(MoveList *list, Move move);
Status remove_last_move(MoveList *list);
Move *get_move(MoveList *list, int index);
int get_move_count(MoveList *list);
int is_in_check(const Position *position, Color color);
int is_insufficient_material(const Position *position);

} // namespace ac
#endif
