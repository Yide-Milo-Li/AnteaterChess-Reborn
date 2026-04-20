#ifndef CHESS_CLI_FEEDBACK_H
#define CHESS_CLI_FEEDBACK_H

#include "error/error_code.h"

int cliShowErrorMessage(ErrorCode code);
int cliShowDisabledFeatureMessage(const char *featureName);

#endif
