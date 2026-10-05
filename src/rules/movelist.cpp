#include "anteater/rules.h"
void ac_init_move_list(AcMoveList *l) {
    if (l) {
        l->count = 0;
        l->status = AC_OK;
    }
}
int ac_add_move(AcMoveList *l, AcMove m) {
    if (!l)
        return AC_INVALID_ARGUMENT;
    if (l->status != AC_OK)
        return l->status;
    if (l->count >= AC_MAX_MOVES) {
        l->status = AC_CAPACITY;
        return AC_CAPACITY;
    }
    l->moves[l->count++] = m;
    return AC_OK;
}
AcMove *ac_get_move(AcMoveList *l, int i) {
    return l && i >= 0 && i < l->count ? &l->moves[i] : NULL;
}
int ac_get_move_count(AcMoveList *l) {
    return l ? l->count : 0;
}
int ac_remove_last_move(AcMoveList *l) {
    if (!l || l->count <= 0)
        return AC_INVALID_ARGUMENT;
    --l->count;
    return AC_OK;
}
