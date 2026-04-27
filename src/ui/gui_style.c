#include "gui_internal.h"

void gui_install_style(void) {
    GtkCssProvider *provider;
    GdkScreen *screen;

    static const char *css =
        "GtkWindow, window { background-color: #211f1c; } "
        "GtkDialog, dialog, messagedialog { background-color: #211f1c; } "
        ".app-window { background-color: #211f1c; } "
        ".app-root { color: #eee3cf; } "
        "label { color: #eee3cf; } "
        "tooltip { background-color: #171513; border: 1px solid #c79a3f; border-radius: 4px; } "
        "tooltip label { color: #f7eedc; padding: 6px 8px; } "
        "button { color: #f4ead7; background-image: none; background-color: #34302a; border: 1px solid #6b604e; border-radius: 6px; padding: 8px 12px; } "
        "button label { color: #f4ead7; } "
        "button:hover { background-color: #433d34; border-color: #c79a3f; } "
        "button:active { background-color: #5a4a34; border-color: #e1ba63; } "
        "button:disabled, button:disabled label { color: #82796a; background-color: #2a2723; border-color: #454039; } "
        "button.primary-button { color: #211f1c; background-color: #c79a3f; border-color: #e1ba63; font-weight: bold; } "
        "button.primary-button label { color: #211f1c; } "
        "button.primary-button:hover { background-color: #d8aa4c; border-color: #f0cd7a; } "
        "button.primary-button:disabled, button.primary-button:disabled label { color: #625844; background-color: #7b6845; border-color: #6d6048; } "
        "button.ai-status-button, button.ai-status-button:disabled { color: #f5e8c8; background-color: #3c4634; border-color: #6f875d; font-weight: bold; } "
        "button.ai-status-button label, button.ai-status-button:disabled label { color: #f5e8c8; } "
        ".hint-button { background-color: #322f28; border-color: #8f7845; } "
        ".hint-button:hover { background-color: #453d2b; border-color: #d8aa4c; } "
        ".destructive-button { color: #f8ded8; background-color: #3e2422; border-color: #9b3b36; } "
        ".destructive-button label { color: #f8ded8; } "
        ".destructive-button:hover { background-color: #522b28; border-color: #c55a50; } "
        "entry, spinbutton { color: #f4ead7; background-color: #171513; border: 1px solid #6b604e; border-radius: 5px; padding: 7px 8px; } "
        "entry:focus, spinbutton:focus { border-color: #c79a3f; box-shadow: inset 0 0 0 1px #c79a3f; } "
        "entry:disabled, spinbutton:disabled { color: #82796a; background-color: #26231f; border-color: #454039; } "
        "radiobutton { color: #eee3cf; } "
        "radiobutton label { color: #eee3cf; } "
        ".panel, .menu-panel, .setup-group, .board-panel, .match-bar { background-color: #2a2723; border: 1px solid #5a5144; border-radius: 7px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.28); } "
        ".panel { padding: 14px; } "
        ".menu-panel { padding: 24px; } "
        ".setup-panel { padding: 22px; } "
        ".setup-group { padding: 14px; } "
        ".history-card { background-color: #2b2823; } "
        ".move-entry-panel { background-color: #26231f; } "
        ".panel-title { color: #f1dfb8; font-weight: bold; } "
        ".app-title { color: #f4ead7; font-size: 20px; font-weight: bold; } "
        ".section-label { color: #d5c7ad; font-weight: bold; } "
        ".caption-label, .ai-summary, .coordinate-label, .match-meta { color: #a99b82; font-size: 12px; } "
        ".segmented-row, .action-row { background-color: transparent; } "
        ".clock-text { color: #d8aa4c; font-weight: bold; } "
        ".history-panel { background-color: #191714; border: 1px solid #4c463c; border-radius: 6px; } "
        ".history-view, textview, textview text { color: #e8ddc9; background-color: #191714; font-size: 12px; } "
        ".status-normal, .status-busy, .status-error, .status-success { border-radius: 6px; padding: 8px 10px; } "
        ".status-normal { color: #bdb09a; background-color: #24211d; border: 1px solid #454039; } "
        ".status-busy { color: #f5e8c8; background-color: #35402f; border: 1px solid #6f875d; } "
        ".status-success { color: #dff0cf; background-color: #263a27; border: 1px solid #6f875d; } "
        ".status-error { color: #f8ded8; background-color: #3e2422; border: 1px solid #9b3b36; } "
        ".format-help { background-color: #34302a; border: 1px solid #6b604e; border-radius: 999px; padding: 4px; } "
        ".format-help:hover { background-color: #453d2b; border-color: #c79a3f; } "
        ".info-icon { color: #d8aa4c; font-weight: bold; } "
        ".match-bar { padding: 10px 12px; } "
        ".turn-banner { color: #211f1c; font-size: 18px; font-weight: bold; padding: 7px 16px; border-radius: 6px; background-color: #d8aa4c; border: 1px solid #f0cd7a; } "
        ".game-shell { background-color: transparent; } "
        ".sidebar { background-color: transparent; } "
        ".board-area { background-color: transparent; } "
        ".board-panel { background-color: #2a241d; padding: 14px; } "
        ".timer-label { color: #d5c7ad; font-weight: bold; } "
        ".board-frame { background-color: #171513; border: 2px solid #7d6841; border-radius: 6px; padding: 6px; box-shadow: 0 10px 28px rgba(0, 0, 0, 0.36); } "
        ".board-grid { background-color: #171513; } "
        ".light-square { background-color: #ead8b6; } "
        ".dark-square { background-color: #946437; } "
        ".piece-fallback { color: #171513; font-size: 28px; font-weight: bold; } "
        ".highlight-from { box-shadow: inset 0 0 0 3px rgba(216, 170, 76, 0.95), inset 0 0 14px rgba(216, 170, 76, 0.35); } "
        ".highlight-destination { box-shadow: inset 0 0 0 3px rgba(94, 122, 90, 0.95), inset 0 0 14px rgba(94, 122, 90, 0.35); } "
        ".highlight-selected { box-shadow: inset 0 0 0 4px rgba(241, 223, 184, 0.98), inset 0 0 16px rgba(216, 170, 76, 0.45); } "
        ".move-input-valid { box-shadow: inset 0 0 0 2px #6f875d; } "
        ".move-input-invalid { box-shadow: inset 0 0 0 2px #c55a50; } ";

    provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css, -1, NULL);
    screen = gdk_screen_get_default();
    if (screen != NULL) {
        gtk_style_context_add_provider_for_screen(screen,
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    g_object_unref(provider);
}
