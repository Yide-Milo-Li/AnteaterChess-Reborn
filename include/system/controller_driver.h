#ifndef CHESS_SYSTEM_CONTROLLER_DRIVER_H
#define CHESS_SYSTEM_CONTROLLER_DRIVER_H

#include "system/controller.h"
#include "system/event.h"

/* Advanced/compatibility driver APIs. Prefer controller.h frontend helpers
 * unless the caller must observe one processed event at a time. */
int controllerEnqueueEvent(Controller *controller, Event event);
int controllerTick(Controller *controller, Event *processedEvent);
int controllerRunUntilIdle(Controller *controller);

#endif
