#include "time/clock.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdint.h>
#include <time.h>

/*
 * Alignment assumptions for future extensions:
 * - The clock is a process-wide gameplay stopwatch, not GameState-owned data.
 * - Controller/FSM must pause it outside GAMEPLAY_STATE and resume it when play returns.
 * - Other services may read elapsed time, but only this module owns pause bookkeeping.
 */

static int64_t startMilliseconds;
static int64_t pauseStartMilliseconds;
static int64_t pausedMilliseconds;
static int clockInitialized = 0;
static int clockPaused = 0;

#if !defined(_WIN32) && !defined(CLOCK_MONOTONIC)
/* Return the current wall-clock value or an error sentinel. */
static time_t get_wall_time(void) {
    return time(NULL);
}
#endif

/* Start or restart the global gameplay stopwatch. */
int initClock(void) {
    int64_t now;

    if (getMonotonicMilliseconds(&now) != 0) {
        clockInitialized = 0;
        clockPaused = 0;
        startMilliseconds = 0;
        pauseStartMilliseconds = 0;
        pausedMilliseconds = 0;
        return 1;
    }

    startMilliseconds = now;
    pauseStartMilliseconds = now;
    pausedMilliseconds = 0;
    clockInitialized = 1;
    clockPaused = 0;
    return 0;
}

/* Validate that the clock has been initialized before callers depend on it. */
int updateClock(void) {
    int64_t now;

    if (!clockInitialized || getMonotonicMilliseconds(&now) != 0) {
        return 1;
    }

    return 0;
}

int getMonotonicMilliseconds(int64_t *outMilliseconds) {
    if (outMilliseconds == NULL) {
        return 1;
    }

#ifdef _WIN32
    {
        LARGE_INTEGER counter;
        LARGE_INTEGER frequency;

        if (!QueryPerformanceFrequency(&frequency)
            || !QueryPerformanceCounter(&counter)
            || frequency.QuadPart <= 0) {
            return 1;
        }

        *outMilliseconds = (int64_t)((counter.QuadPart * 1000) / frequency.QuadPart);
        return 0;
    }
#elif defined(CLOCK_MONOTONIC)
    {
        struct timespec now;

        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
            return 1;
        }

        *outMilliseconds = ((int64_t)now.tv_sec * 1000) + ((int64_t)now.tv_nsec / 1000000);
        return 0;
    }
#else
    {
        time_t now = get_wall_time();

        if (now == (time_t)-1) {
            return 1;
        }

        *outMilliseconds = (int64_t)now * 1000;
        return 0;
    }
#endif
}

/* Freeze elapsed-time accumulation while the program is outside gameplay. */
int pauseClock(void) {
    int64_t now;

    if (!clockInitialized) {
        return 1;
    }

    if (clockPaused) {
        return 0;
    }

    if (getMonotonicMilliseconds(&now) != 0) {
        return 1;
    }

    pauseStartMilliseconds = now;
    clockPaused = 1;
    return 0;
}

/* Resume elapsed-time accumulation after the clock has been paused. */
int resumeClock(void) {
    int64_t now;
    int64_t pausedDelta;

    if (!clockInitialized) {
        return 1;
    }

    if (!clockPaused) {
        return 0;
    }

    if (getMonotonicMilliseconds(&now) != 0) {
        return 1;
    }

    pausedDelta = now - pauseStartMilliseconds;
    if (pausedDelta > 0) {
        if (pausedMilliseconds > INT64_MAX - pausedDelta) {
            pausedMilliseconds = INT64_MAX;
        } else {
            pausedMilliseconds += pausedDelta;
        }
    }

    clockPaused = 0;
    return 0;
}

int64_t getElapsedTimeMilliseconds(void) {
    int64_t now;
    int64_t elapsed;

    if (!clockInitialized) {
        return 0;
    }

    if (clockPaused) {
        now = pauseStartMilliseconds;
    } else if (getMonotonicMilliseconds(&now) != 0) {
        return 0;
    }

    if (now <= startMilliseconds) {
        return 0;
    }

    elapsed = now - startMilliseconds;
    if (elapsed <= pausedMilliseconds) {
        return 0;
    }

    return elapsed - pausedMilliseconds;
}

/* Return total gameplay elapsed time in seconds, excluding paused intervals. */
int64_t getElapsedTimeSeconds(void) {
    return getElapsedTimeMilliseconds() / 1000;
}
