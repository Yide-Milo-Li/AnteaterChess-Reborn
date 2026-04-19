#include <assert.h>
#include <time.h>

#include "time/clock.h"

/*
 * Alignment assumptions for future extensions:
 * - The clock module is a standalone gameplay stopwatch shared by services.
 * - These tests lock pause/resume behavior rather than any controller-specific transition logic.
 * - Elapsed time is second-granularity, so tests wait for whole-second changes.
 */

/* Spin until the elapsed clock reading changes or a hard loop cap is reached. */
static int wait_for_elapsed_change(int baseline) {
    time_t deadline = time(NULL) + 3;

    while (time(NULL) <= deadline) {
        int current = getElapsedTimeSeconds();

        if (current != baseline) {
            return current;
        }
    }

    return baseline;
}

/* Verify initialization guards and monotonic elapsed-time updates. */
static void test_clock_initialization_and_progress(void) {
    int baseline;
    int advanced;

    assert(getElapsedTimeSeconds() == 0);
    assert(updateClock() != 0);
    assert(initClock() == 0);
    assert(updateClock() == 0);

    baseline = getElapsedTimeSeconds();
    advanced = wait_for_elapsed_change(baseline);
    assert(advanced >= baseline);
    assert(advanced != baseline);
}

/* Verify paused intervals do not contribute to gameplay elapsed time. */
static void test_clock_pause_and_resume(void) {
    int beforePause;
    int whilePaused;
    int afterResume;

    assert(initClock() == 0);
    beforePause = getElapsedTimeSeconds();
    assert(pauseClock() == 0);
    whilePaused = wait_for_elapsed_change(beforePause);
    assert(whilePaused == beforePause);

    assert(resumeClock() == 0);
    afterResume = wait_for_elapsed_change(beforePause);
    assert(afterResume > beforePause);
}

/* Run the global clock regression suite. */
int main(void) {
    test_clock_initialization_and_progress();
    test_clock_pause_and_resume();
    return 0;
}
