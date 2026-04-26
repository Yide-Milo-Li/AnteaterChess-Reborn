#ifndef CHESS_SYSTEM_CONTROLLER_H
#define CHESS_SYSTEM_CONTROLLER_H

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "error/error_code.h"
#include "input/move_request.h"
#include "system/event.h"
#include "system/event_queue.h"

/*
 * Public integration surface:
 * - New frontends should hold a Controller and use the preferred frontend
 *   helpers below. Those helpers express user intent and hide queue/FSM details.
 * - Controller owns runtime orchestration: queue priority, idle draining,
 *   lifecycle advancement, timer checks, and AI work scheduling.
 * - FSM owns event semantics, transition legality, and gameplay mutations.
 * - Low-level driver APIs remain exported for compatibility tests and callers
 *   that genuinely need step-by-step event feedback.
 */
typedef struct {
    GameState state;
    EventQueue queue;
} Controller;

void initController(Controller *controller, const GameConfig *config);
const GameState *controllerGetState(const Controller *controller);

/* Advanced/compatibility driver APIs. Prefer the frontend helpers below unless
 * the caller must observe one processed event at a time. */
int controllerEnqueueEvent(Controller *controller, Event event);
int controllerTick(Controller *controller, Event *processedEvent);
int controllerRunUntilIdle(Controller *controller);

/* Preferred frontend APIs. These methods accept UI/CLI intent, drive the
 * controller to the next stable state, and avoid direct FSM coupling. */
int controllerStartConfiguredGame(Controller *controller, const GameConfig *config);
int controllerRequestNewGame(Controller *controller);
int controllerRequestBack(Controller *controller);
int controllerRequestExit(Controller *controller);
int controllerMoveRequestNeedsPromotion(const Controller *controller,
                                        MoveRequest request,
                                        int *needsPromotion);
int controllerSubmitMoveRequestDetailed(Controller *controller,
                                        MoveRequest request,
                                        ErrorCode *errorCode);
int controllerSubmitMoveRequest(Controller *controller, MoveRequest request);
int controllerSubmitMove(Controller *controller, Command command);
int controllerRequestUndo(Controller *controller);
int controllerRequestLeaveGame(Controller *controller);
int controllerGetHint(const Controller *controller, Move *move);

/* Legacy wrapper retained for older integrations. */
int runGameLoop(GameState *state);

#endif
