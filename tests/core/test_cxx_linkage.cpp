#include "anteater/ai.hpp"
#include "anteater/session.hpp"
#include <cassert>

using namespace ac;
static int64_t now(void *) {
    return 0;
}
int main() {
    Position p{};
    position_init(&p);
    assert(p.hash == position_hash(&p));
    SessionOptions options{};
    options.clock = {now, nullptr};
    Session *s = session_create(&options);
    assert(s);
    GameConfig c{};
    init_default_game_config(&c);
    assert(session_start(s, &c) == Status::Ok);
    session_destroy(s);
    return 0;
}
