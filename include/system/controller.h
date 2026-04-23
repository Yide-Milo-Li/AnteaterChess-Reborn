#ifndef CHESS_SYSTEM_CONTROLLER_H
#define CHESS_SYSTEM_CONTROLLER_H

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "system/event.h"
#include "system/event_queue.h"

/*
 * Public integration surface:
 * - Future UI layers should prefer holding a Controller and integrating
 *   through this header instead of calling the FSM directly.
 * - Controller owns runtime orchestration concerns such as event routing,
 *   queue draining, and controller-synthesized work.
 * - FSM remains the internal compatibility engine behind this surface.
 * - runGameLoop() stays exported as a legacy wrapper for older callers.
 */
typedef struct {
    GameState state;
    EventQueue queue;
} Controller;

void initController(Controller *controller, const GameConfig *config);
const GameState *controllerGetState(const Controller *controller);
int controllerEnqueueEvent(Controller *controller, Event event);
int controllerTick(Controller *controller, Event *processedEvent);
int controllerRunUntilIdle(Controller *controller);
int controllerStartConfiguredGame(Controller *controller, const GameConfig *config);
int controllerRequestNewGame(Controller *controller);
int controllerRequestBack(Controller *controller);
int controllerRequestExit(Controller *controller);
int controllerSubmitMove(Controller *controller, Command command);
int controllerRequestUndo(Controller *controller);
int controllerRequestLeaveGame(Controller *controller);
int controllerGetHint(const Controller *controller, Move *move);
int runGameLoop(GameState *state);

#endif
