#include "ui/fatal_error_window.h"

#include <gtk/gtk.h>

#include "error/error.h"

int showFatalError(ErrorCode code) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s",
        getErrorMessage(code));

    gtk_window_set_title(GTK_WINDOW(dialog), "Anteater Chess");
    (void)gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return 0;
}
