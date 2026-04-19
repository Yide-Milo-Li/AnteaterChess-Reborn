#include <time.h>

#include "time/clock.h"

static time_t startTime;
/* Distinguish "clock not started yet" from a real timestamp so callers do not
 * accidentally read a huge Unix-time delta before initialization. */
static int clockInitialized = 0;

int initClock(void) {
    time_t now = time(NULL);

    if (now == (time_t) -1) {
        clockInitialized = 0;
        return 0;
    }

    startTime = now;
    clockInitialized = 1;
    return 1;
}

int updateClock(void) {
    /* The current clock is derived from wall time on demand, so there is no
     * per-tick state to refresh yet. Keeping the hook preserves the header API. */
    return clockInitialized;
}

int getElapsedTimeSeconds(void) {
    time_t currentTime = time(NULL);

    if (!clockInitialized || currentTime == (time_t) -1) {
        return 0;
    }

    return (int) difftime(currentTime, startTime);
}
