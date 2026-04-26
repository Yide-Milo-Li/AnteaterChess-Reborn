#ifndef CHESS_SYSTEM_CONTROLLER_H
#define CHESS_SYSTEM_CONTROLLER_H

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/move.h"
#include "error/error_code.h"
#include "input/move_request.h"
#include "system/event_queue.h"

/*
 * Public integration surface:
 * - New frontends should hold a Controller and use the preferred frontend
 *   helpers below. Those helpers express user intent and hide queue/FSM details.
 * - Controller owns runtime orchestration: queue priority, idle draining,
 *   lifecycle advancement, timer checks, and external move-provider scheduling.
 * - FSM owns event semantics, transition legality, and gameplay mutations.
 * - Low-level driver APIs live in controller_driver.h for compatibility tests
 *   and callers that genuinely need step-by-step event feedback.
 */
typedef int (*ControllerMoveProvider)(const GameState *state,
                                      Move *move,
                                      void *context);

typedef struct {
    GameState state;
    EventQueue queue;
    ControllerMoveProvider moveProvider;
    void *moveProviderContext;
} Controller;

void initController(Controller *controller, const GameConfig *config);
const GameState *controllerGetState(const Controller *controller);
/* Attach an optional external move provider after controller initialization or
 * configured-game startup. Frontends that do not install one should disable AI
 * turns or leave them idle. */
void controllerSetMoveProvider(Controller *controller,
                               ControllerMoveProvider provider,
                               void *context);

/* Preferred frontend APIs. These methods accept UI/CLI intent, drive the
 * controller to the next stable state, and avoid direct FSM coupling. */
int controllerSync(Controller *controller);
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
int controllerRequestUndo(Controller *controller);
int controllerRequestLeaveGame(Controller *controller);
int controllerGetHint(const Controller *controller, Move *move);

#endif
