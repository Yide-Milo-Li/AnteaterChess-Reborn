#include <time.h>
#include "time/clock.h"

/* Global start time for the game */
static time_t startTime;

/* Initialize the clock by recording current system time */
int initClock(void) {
    startTime = time(NULL);
    return 1; // Return 1 for success
}

/* Update logic if needed (placeholder for this simple version) */
int updateClock(void) {
    return 1;
}

/* Calculate and return total seconds since initClock was called */
int getElapsedTimeSeconds(void) {
    time_t currentTime = time(NULL);
    return (int)difftime(currentTime, startTime);
}