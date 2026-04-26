#include <assert.h>
#include <stdint.h>
#include <time.h>

#include "time/clock.h"

/*
 * Alignment assumptions for future extensions:
 * - The clock module is a standalone gameplay stopwatch shared by services.
 * - These tests lock pause/resume behavior rather than any controller-specific transition logic.
 * - Elapsed milliseconds drive the clock; elapsed seconds remain a compatibility view.
 */

static void wait_for_monotonic_delta(int64_t minimumDeltaMilliseconds) {
    int64_t baseline;
    int64_t current;
    time_t deadline = time(NULL) + 3;

    assert(getMonotonicMilliseconds(&baseline) == 0);
    current = baseline;

    while (time(NULL) <= deadline && current - baseline < minimumDeltaMilliseconds) {
        assert(getMonotonicMilliseconds(&current) == 0);
    }

    assert(current - baseline >= minimumDeltaMilliseconds);
}

/* Spin until the elapsed millisecond reading changes or a hard loop cap is reached. */
static int64_t wait_for_elapsed_millisecond_change(int64_t baseline) {
    time_t deadline = time(NULL) + 3;

    while (time(NULL) <= deadline) {
        int64_t current = getElapsedTimeMilliseconds();

        if (current != baseline) {
            return current;
        }
    }

    return baseline;
}

/* Verify initialization guards and monotonic elapsed-time updates. */
static void test_clock_initialization_and_progress(void) {
    int64_t baseline;
    int64_t advanced;

    assert(getElapsedTimeSeconds() == 0);
    assert(getElapsedTimeMilliseconds() == 0);
    assert(updateClock() != 0);
    assert(initClock() == 0);
    assert(updateClock() == 0);

    baseline = getElapsedTimeMilliseconds();
    advanced = wait_for_elapsed_millisecond_change(baseline);
    assert(advanced >= baseline);
    assert(advanced != baseline);
    assert(getElapsedTimeSeconds() == getElapsedTimeMilliseconds() / 1000);
}

/* Verify paused intervals do not contribute to gameplay elapsed time. */
static void test_clock_pause_and_resume(void) {
    int64_t beforePause;
    int64_t whilePaused;
    int64_t afterResume;

    assert(initClock() == 0);
    assert(pauseClock() == 0);
    beforePause = getElapsedTimeMilliseconds();
    wait_for_monotonic_delta(20);
    whilePaused = getElapsedTimeMilliseconds();
    assert(whilePaused == beforePause);

    assert(resumeClock() == 0);
    afterResume = wait_for_elapsed_millisecond_change(beforePause);
    assert(afterResume > beforePause);
}

static void test_monotonic_milliseconds_progress(void) {
    int64_t first;
    int64_t second;

    assert(getMonotonicMilliseconds(&first) == 0);
    assert(getMonotonicMilliseconds(&second) == 0);
    assert(second >= first);
    assert(getMonotonicMilliseconds(NULL) != 0);
}

/* Run the global clock regression suite. */
int main(void) {
    test_clock_initialization_and_progress();
    test_clock_pause_and_resume();
    test_monotonic_milliseconds_progress();
    return 0;
}
