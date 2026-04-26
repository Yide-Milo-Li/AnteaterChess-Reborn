#ifndef CHESS_INPUT_MOVE_REQUEST_PARSER_H
#define CHESS_INPUT_MOVE_REQUEST_PARSER_H

#include "input/move_request.h"

/* Parse two coordinate text fields directly into a frontend MoveRequest. */
int parseMoveRequestFields(const char *fromText,
                           const char *toText,
                           PromotionChoice promotion,
                           MoveRequest *request);

#endif
