#include "ui/fatal_error_window.h"

#include <gtk/gtk.h>

#include "error/error.h"
#include "gui_internal.h"

int showFatalError(ErrorCode code) {
    gui_show_message_dialog(NULL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "Anteater Chess",
        getErrorMessage(code));
    return 0;
}
