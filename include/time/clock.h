#ifndef CHESS_TIME_CLOCK_H
#define CHESS_TIME_CLOCK_H

/* Header modification: pause/resume were added so controller/FSM can stop the
 * gameplay clock while menus are active without storing clock runtime state in
 * GameState. */
int initClock(void);
int updateClock(void);
int getElapsedTimeSeconds(void);
int pauseClock(void);
int resumeClock(void);

#endif
