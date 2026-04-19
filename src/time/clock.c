#include "time/clock.h"

#include <stdint.h>
#include <time.h>

/*
 * Alignment assumptions for future extensions:
 * - The clock is a process-wide gameplay stopwatch, not GameState-owned data.
 * - Controller/FSM must pause it outside GAMEPLAY_STATE and resume it when play returns.
 * - Other services may read elapsed time, but only this module owns pause bookkeeping.
 */

static time_t startTime;
static time_t pauseStartTime;
static int64_t pausedSeconds;
static int clockInitialized = 0;
static int clockPaused = 0;

/* Return the current wall-clock value or an error sentinel. */
static time_t get_wall_time(void) {
    return time(NULL);
}

/* Convert elapsed whole seconds into a stable HH:MM:SS-friendly integer value. */
static int64_t compute_elapsed_seconds(time_t now) {
    double seconds = difftime(now, startTime);

    if (seconds <= 0.0) {
        return 0;
    }

    if (seconds >= (double) INT64_MAX) {
        return INT64_MAX - pausedSeconds;
    }

    return (int64_t) seconds - pausedSeconds;
}

/* Start or restart the global gameplay stopwatch. */
int initClock(void) {
    time_t now = get_wall_time();

    if (now == (time_t) -1) {
        clockInitialized = 0;
        clockPaused = 0;
        pausedSeconds = 0;
        return 1;
    }

    startTime = now;
    pauseStartTime = now;
    pausedSeconds = 0;
    clockInitialized = 1;
    clockPaused = 0;
    return 0;
}

/* Validate that the clock has been initialized before callers depend on it. */
int updateClock(void) {
    if (!clockInitialized || get_wall_time() == (time_t) -1) {
        return 1;
    }

    return 0;
}

/* Freeze elapsed-time accumulation while the program is outside gameplay. */
int pauseClock(void) {
    time_t now;

    if (!clockInitialized) {
        return 1;
    }

    if (clockPaused) {
        return 0;
    }

    now = get_wall_time();
    if (now == (time_t) -1) {
        return 1;
    }

    pauseStartTime = now;
    clockPaused = 1;
    return 0;
}

/* Resume elapsed-time accumulation after the clock has been paused. */
int resumeClock(void) {
    time_t now;

    if (!clockInitialized) {
        return 1;
    }

    if (!clockPaused) {
        return 0;
    }

    now = get_wall_time();
    if (now == (time_t) -1) {
        return 1;
    }

    {
        double pausedDelta = difftime(now, pauseStartTime);

        if (pausedDelta > 0.0) {
            if (pausedDelta >= (double) INT64_MAX || pausedSeconds > INT64_MAX - (int64_t) pausedDelta) {
                pausedSeconds = INT64_MAX;
            } else {
                pausedSeconds += (int64_t) pausedDelta;
            }
        }
    }
    clockPaused = 0;
    return 0;
}

/* Return total gameplay elapsed time in seconds, excluding paused intervals. */
int64_t getElapsedTimeSeconds(void) {
    time_t now;

    if (!clockInitialized) {
        return 0;
    }

    if (clockPaused) {
        now = pauseStartTime;
    } else {
        now = get_wall_time();
        if (now == (time_t) -1) {
            return 0;
        }
    }

    if (now < startTime) {
        return 0;
    }

    return compute_elapsed_seconds(now);
}
