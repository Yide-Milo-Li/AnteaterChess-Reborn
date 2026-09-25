#ifndef ANTEATER_RULES_H
#define ANTEATER_RULES_H
#include "anteater/types.h"
void ac_position_init(AcPosition *position);
uint64_t ac_position_hash(const AcPosition *position);
AcStatus ac_position_apply(AcPosition *position, AcMove move, AcUndo *undo);
AcStatus ac_position_unmake(AcPosition *position, const AcUndo *undo);
AcStatus ac_position_result(const AcPosition *position, AcGameResult *result);

int ac_generate_moves(const AcPosition *state, AcMoveList *list);
int ac_generate_legal_moves(const AcPosition *state, AcMoveList *list);
int ac_generate_legal_moves_for_position(const AcPosition *state, AcSquare from, AcMoveList *list);

int ac_resolve_move_request(const AcPosition *state, AcMoveRequest request, AcMove *resolvedMove);

typedef enum { AC_SELECT_VALID, AC_SELECT_EMPTY, AC_SELECT_OPPONENT_PIECE, AC_SELECT_OUT_OF_BOUNDS } AcSelectionResult;

int ac_validate_move(const AcPosition *state, AcMove move);
AcSelectionResult ac_validate_selection(const AcPosition *state, AcSquare pos);

/* Parse two coordinate text fields directly into a frontend AcMoveRequest. */
int ac_parse_move_request_fields(const char *fromText, const char *toText, AcPromotionChoice promotion,
                                 AcMoveRequest *request);

void ac_init_move_list(AcMoveList *list);
int ac_add_move(AcMoveList *list, AcMove move);
int ac_remove_last_move(AcMoveList *list);
AcMove *ac_get_move(AcMoveList *list, int index);
int ac_get_move_count(AcMoveList *list);
int ac_is_in_check(const AcPosition *position, AcColor color);
int ac_is_insufficient_material(const AcPosition *position);
#endif
