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
    auto owner = Session::create(options);
    Session *s = std::get_if<Session>(&owner);
    assert(s);
    GameConfig c{};
    init_default_game_config(&c);
    assert(s->start(c) == Status::Ok);
    return 0;
}
