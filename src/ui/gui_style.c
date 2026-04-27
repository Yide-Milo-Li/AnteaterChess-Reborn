#include "gui_internal.h"

void gui_install_style(void) {
    GtkCssProvider *provider;
    GdkScreen *screen;

    static const char *css =
        "GtkWindow, window { background-color: #171717; } "
        "GtkDialog, dialog, messagedialog { background-color: #171717; } "
        ".app-window { background-color: #171717; } "
        ".app-root { color: #f8f5ee; } "
        "label { color: #f8f5ee; } "
        "tooltip { background-color: #080808; border: 1px solid #f0c15f; border-radius: 4px; } "
        "tooltip label { color: #ffffff; padding: 6px 8px; } "
        "button { color: #fffaf0; background-image: none; background-color: #303030; border: 1px solid #8f846d; border-radius: 6px; padding: 8px 12px; } "
        "button label { color: #fffaf0; } "
        "button:hover { background-color: #424242; border-color: #f0c15f; } "
        "button:active { background-color: #564325; border-color: #ffd987; } "
        "button:disabled, button:disabled label { color: #b8b1a3; background-color: #252525; border-color: #5a5348; } "
        "button.primary-button { color: #141414; background-color: #f0c15f; border-color: #ffd987; font-weight: bold; } "
        "button.primary-button label { color: #141414; } "
        "button.primary-button:hover { background-color: #ffd071; border-color: #ffe1a2; } "
        "button.primary-button:disabled, button.primary-button:disabled label { color: #39301e; background-color: #9f7f3d; border-color: #8b7446; } "
        "button.ai-status-button, button.ai-status-button:disabled { color: #f7ffe9; background-color: #334829; border-color: #8cc36b; font-weight: bold; } "
        "button.ai-status-button label, button.ai-status-button:disabled label { color: #f7ffe9; } "
        ".hint-button { background-color: #2f2f2f; border-color: #b99851; } "
        ".hint-button:hover { background-color: #463a22; border-color: #ffd071; } "
        ".fullscreen-button { color: #f8f5ee; background-color: #252525; border-color: #9f9278; } "
        ".fullscreen-button label { color: #f8f5ee; } "
        ".fullscreen-button:hover { background-color: #363636; border-color: #f0c15f; } "
        ".destructive-button { color: #fff1ed; background-color: #4d2421; border-color: #de6a5f; } "
        ".destructive-button label { color: #fff1ed; } "
        ".destructive-button:hover { background-color: #642b27; border-color: #ff8b80; } "
        "entry, spinbutton { color: #ffffff; background-color: #0d0d0d; border: 1px solid #8f846d; border-radius: 5px; padding: 7px 8px; } "
        "entry:focus, spinbutton:focus { border-color: #f0c15f; box-shadow: inset 0 0 0 1px #f0c15f; } "
        "entry:disabled, spinbutton:disabled { color: #c6beb0; background-color: #252525; border-color: #5a5348; } "
        "radiobutton { color: #f8f5ee; } "
        "radiobutton label { color: #f8f5ee; } "
        ".panel, .menu-panel, .setup-group, .board-panel, .match-bar { background-color: #242424; border: 1px solid #6f6656; border-radius: 7px; box-shadow: 0 8px 24px rgba(0, 0, 0, 0.34); } "
        ".panel { padding: 14px; } "
        ".menu-panel { padding: 24px; } "
        ".setup-panel { padding: 22px; } "
        ".setup-group { padding: 14px; } "
        ".history-card { background-color: #262626; } "
        ".move-entry-panel { background-color: #202020; } "
        ".panel-title { color: #ffe0a3; font-weight: bold; } "
        ".app-title { color: #ffffff; font-size: 20px; font-weight: bold; } "
        ".section-label { color: #f3e5c5; font-weight: bold; } "
        ".caption-label, .ai-summary, .coordinate-label, .match-meta { color: #ddd3c0; font-size: 12px; } "
        ".segmented-row, .action-row { background-color: transparent; } "
        ".clock-text { color: #ffd071; font-weight: bold; } "
        ".history-panel { background-color: #0f0f0f; border: 1px solid #6f6656; border-radius: 6px; } "
        ".history-view, textview, textview text { color: #f7f1e6; background-color: #0f0f0f; font-size: 12px; } "
        ".status-normal, .status-busy, .status-error, .status-success { border-radius: 6px; padding: 8px 10px; } "
        ".status-normal { color: #f0e6d2; background-color: #1f1f1f; border: 1px solid #5e574b; } "
        ".status-busy { color: #f8ffe8; background-color: #2e4724; border: 1px solid #8cc36b; } "
        ".status-success { color: #edffe2; background-color: #244022; border: 1px solid #82c66d; } "
        ".status-error { color: #fff1ed; background-color: #4d2421; border: 1px solid #de6a5f; } "
        ".format-help { background-color: #303030; border: 1px solid #8f846d; border-radius: 999px; padding: 4px; } "
        ".format-help:hover { background-color: #463a22; border-color: #f0c15f; } "
        ".info-icon { color: #ffd071; font-weight: bold; } "
        ".match-bar { padding: 10px 12px; } "
        ".turn-banner { color: #111111; font-size: 18px; font-weight: bold; padding: 7px 16px; border-radius: 6px; background-color: #ffd071; border: 1px solid #ffe1a2; } "
        ".game-shell { background-color: transparent; } "
        ".sidebar { background-color: transparent; } "
        ".board-area { background-color: transparent; } "
        ".board-panel { background-color: #211d18; padding: 14px; } "
        ".timer-label { color: #f3e5c5; font-weight: bold; } "
        ".board-frame { background-color: #090909; border: 2px solid #a88a4d; border-radius: 6px; padding: 6px; box-shadow: 0 10px 28px rgba(0, 0, 0, 0.42); } "
        ".board-grid { background-color: #090909; } "
        ".light-square { background-color: #ead8b6; } "
        ".dark-square { background-color: #946437; } "
        ".piece-fallback { color: #171513; font-size: 28px; font-weight: bold; } "
        ".highlight-from { box-shadow: inset 0 0 0 3px rgba(255, 208, 113, 0.98), inset 0 0 14px rgba(255, 208, 113, 0.42); } "
        ".highlight-destination { box-shadow: inset 0 0 0 3px rgba(120, 218, 106, 0.98), inset 0 0 14px rgba(120, 218, 106, 0.42); } "
        ".highlight-selected { box-shadow: inset 0 0 0 4px rgba(255, 247, 230, 0.98), inset 0 0 16px rgba(255, 208, 113, 0.52); } "
        ".move-input-valid { box-shadow: inset 0 0 0 2px #82c66d; } "
        ".move-input-invalid { box-shadow: inset 0 0 0 2px #ff8b80; } ";

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
