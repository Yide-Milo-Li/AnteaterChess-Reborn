#include "platform.h"
#include <glib.h>

int64_t ac_platform_now(void *unused) {
    (void)unused;
    return g_get_monotonic_time() / 1000;
}
