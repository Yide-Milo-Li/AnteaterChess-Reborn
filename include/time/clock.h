#ifndef CHESS_TIME_CLOCK_H
#define CHESS_TIME_CLOCK_H

#include <stdint.h>

/* Header modification: pause/resume were added so controller/FSM can stop the
 * gameplay clock while menus are active without storing clock runtime state in
 * GameState. */
int initClock(void);
int updateClock(void);
/* Header modification: elapsed seconds now use int64_t because difftime()
 * may exceed int during long-running sessions, and truncating to int risks
 * overflow in services that consume the gameplay clock. */
int64_t getElapsedTimeSeconds(void);
int pauseClock(void);
int resumeClock(void);

#endif
