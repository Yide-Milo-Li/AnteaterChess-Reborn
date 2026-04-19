#include <assert.h>

#include "time/clock.h"

static void test_clock_initialization_guards(void) {
    int elapsedBeforeInit = getElapsedTimeSeconds();

    assert(elapsedBeforeInit == 0);
    assert(updateClock() == 0);
    assert(initClock() == 1);
    assert(updateClock() == 1);
    assert(getElapsedTimeSeconds() >= 0);
}

int main(void) {
    test_clock_initialization_guards();
    return 0;
}
