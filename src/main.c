#include <stdio.h>

#include "core/gamestate.h"
#include "system/controller.h"
#include "system/event_queue.h"
#include "system/fsm.h"
#include "system/system_state.h"

static const EventSources EMPTY_SOURCES = {0};

int main(void) {
    GameState  state;
    EventQueue queue;

    initGameState(&state);
    initEventQueue(&queue);
    initFSM();

    printf("[AnteaterChess] Phase E skeleton starting.\n");
    printf("[AnteaterChess] Initial state: %s\n",
           systemStateName(getSystemState()));

    {
        Event exitEvt = createSystemEvent(EVENT_EXIT_PROGRAM);
        enqueueEvent(&queue, exitEvt, queueForEvent(EVENT_EXIT_PROGRAM));
    }

    int rc = runGameLoop(&state, &queue, &EMPTY_SOURCES);

    printf("[AnteaterChess] Final state: %s (rc=%d)\n",
           systemStateName(getSystemState()), rc);
    return rc;
}