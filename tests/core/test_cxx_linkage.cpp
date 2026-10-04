#include "anteater/ai.h"
#include "anteater/session.h"
#include <cassert>
static int64_t now(void *) { return 0; }
int main() {
    AcPosition p{};
    ac_position_init(&p);
    assert(p.hash == ac_position_hash(&p));
    AcSessionOptions options{};
    options.clock = {now, nullptr};
    AcSession *s = ac_session_create(&options);
    assert(s);
    AcGameConfig c{};
    ac_init_default_game_config(&c);
    assert(ac_session_start(s, &c) == AC_OK);
    ac_session_destroy(s);
    return 0;
}
