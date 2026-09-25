#ifndef AC_PLATFORM_H
#define AC_PLATFORM_H
#include "anteater/session.h"
typedef struct AcPlatformLog AcPlatformLog;
int64_t ac_platform_now(void *unused);
AcPlatformLog *ac_platform_log_create(const char *directory);
AcStatus ac_platform_log_write(void *context, const AcSnapshot *snapshot);
const char *ac_platform_log_path(const AcPlatformLog *log);
void ac_platform_log_destroy(AcPlatformLog *log);
void ac_platform_prepare_runtime(void);
#endif
