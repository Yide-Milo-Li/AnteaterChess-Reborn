#include "anteater/rules.hpp"

namespace ac {
void init_move_list(MoveList *l) {
    if (l) {
        l->count = 0;
        l->status = Status::Ok;
    }
}
Status add_move(MoveList *l, Move m) {
    if (!l)
        return Status::InvalidArgument;
    if (l->status != Status::Ok)
        return l->status;
    if (l->count >= MaxMoves) {
        l->status = Status::Capacity;
        return Status::Capacity;
    }
    l->moves[l->count++] = m;
    return Status::Ok;
}
Move *get_move(MoveList *l, int i) {
    return l && i >= 0 && i < l->count ? &l->moves[i] : NULL;
}
int get_move_count(MoveList *l) {
    return l ? l->count : 0;
}
Status remove_last_move(MoveList *l) {
    if (!l || l->count <= 0)
        return Status::InvalidArgument;
    --l->count;
    return Status::Ok;
}

} // namespace ac
