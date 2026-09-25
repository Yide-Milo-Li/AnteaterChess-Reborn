#ifndef CHESS_UI_GUI_INTERNAL_H
#define CHESS_UI_GUI_INTERNAL_H

#include "anteater/session.h"
#include "anteater/ai.h"
#include "platform.h"
#include <stdint.h>
#include <gtk/gtk.h>

typedef enum {
    AC_INIT_STATE,
    AC_MAIN_MENU_STATE,
    AC_GAME_MODE_SELECTION_STATE,
    AC_GAME_SETUP_STATE,
    AC_GAMEPLAY_STATE,
    AC_END_GAME_MENU_STATE,
    AC_GAME_TERMINATION_STATE,
    AC_EXIT_STATE
} GuiPage;

typedef struct Gui Gui;
typedef struct {
    AcPosition position;
    AcBoard board;
    AcColor currentTurn;
    AcGameConfig config;
    AcGameResult result;
    GuiPage systemState;
    struct {
        int type;
    } players[2];
    struct {
        const AcMove *moves;
        int count;
    } moveHistory;
    int moveCount;
    uint64_t revision;
} GuiView;
#define AC_AI 1
#define AC_HUMAN 0

typedef enum {
    AC_ERR_INVALID_INPUT,
    /* Header change: menu flows need a separate code for a parsed number that
     * does not match any visible menu choice. */
    AC_ERR_INVALID_MENU_SELECTION,
    /* Header change: move-entry callers need one stable code for malformed
     * FROM/TO text without conflating it with move legality. */
    AC_ERR_INVALID_MOVE_FORMAT,
    /* Header change: current validation already detects off-board positions,
     * so the public enum needs a dedicated user-facing result. */
    AC_ERR_POSITION_OUT_OF_BOUNDS,
    AC_ERR_EMPTY_SELECTION,
    AC_ERR_OPPONENT_PIECE,
    AC_ERR_ILLEGAL_MOVE,
    AC_ERR_UNRESOLVED_CHECK,
    /* Header change: setup flows validate turn-length input separately from
     * generic menu parsing, so the public enum needs a timer-specific code. */
    AC_ERR_INVALID_TIMER_SETTING,
    AC_ERR_INVALID_AI_TIMER_SETTING,
    AC_ERR_UNDO_UNAVAILABLE,
    AC_ERR_HINT_UNAVAILABLE,
    AC_ERR_AI_UNAVAILABLE,
    AC_ERR_NOT_YOUR_TURN,
    AC_ERR_TIME_UP,
    AC_ERR_ACTION_UNAVAILABLE,
    AC_ERR_FATAL
} AcErrorCode;

typedef struct GuiAsyncJob GuiAsyncJob;

typedef enum { GUI_STATUS_NORMAL, GUI_STATUS_BUSY, GUI_STATUS_ERROR } GuiStatusKind;

/* 5 difficulty radios in the setup menu:
 * Easy / Medium / Hard / Tournament / Experimental.
 * Experimental currently uses the Hard search policy. */
#define GUI_AI_DIFFICULTY_COUNT 5

struct Gui {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *new_game_button;
    GtkWidget *quit_game_button;
    GtkWidget *board_cells[8][10];
    GtkWidget *board_images[8][10];
    GtkWidget *board_piece_labels[8][10];
    GtkWidget *turn_label;
    GtkWidget *time_display;
    GtkWidget *history_view;
    GtkWidget *history_ai_summary_label;
    GtkWidget *black_timer_label;
    GtkWidget *white_timer_label;
    GtkWidget *status_label;
    GtkWidget *from_entry;
    GtkWidget *to_entry;
    GtkWidget *submit_button;
    GtkWidget *undo_button;
    GtkWidget *hint_button;
    GtkWidget *fullscreen_button;
    GtkWidget *leave_game_button;
    GtkWidget *endgame_dialog;
    GtkWidget *setup_timer_toggle;
    GtkWidget *setup_hours_spin;
    GtkWidget *setup_minutes_spin;
    GtkWidget *setup_seconds_spin;
    GtkWidget *setup_timer_widgets[6];
    GtkWidget *setup_side_white;
    GtkWidget *setup_side_black;
    GtkWidget *setup_ai_diff_buttons[GUI_AI_DIFFICULTY_COUNT];
    GtkWidget *setup_white_diff_buttons[GUI_AI_DIFFICULTY_COUNT];
    GtkWidget *setup_black_diff_buttons[GUI_AI_DIFFICULTY_COUNT];
    GuiAsyncJob *ai_job;
    GuiAsyncJob *hint_job;
    AcSession *session;
    AcPlatformLog *log;
    AcSnapshot snapshot;
    GuiView view;
    GuiPage page;
    AcGameConfig pendingConfig;
    GuiPage last_rendered_state;
    AcColor last_turn;
    int last_move_count;
    unsigned int async_generation;
    int ai_failure_active;
    int ai_failure_move_count;
    AcColor ai_failure_turn;
    int highlight_destinations[8][10];
    AcSquare highlight_from;
    AcSquare hint_from;
    AcSquare hint_to;
    AcColor hint_turn;
    int hint_move_count;
    guint sync_source_id;
    int has_rendered_state;
    int has_highlight_from;
    int has_hint_highlight;
    int is_fullscreen;
    int fullscreen_transition_pending;
    int should_quit;
};

void gui_clear_view_refs(Gui *gui);
void gui_install_style(void);
void gui_rebuild_root_box(Gui *gui, GtkAlign halign, GtkAlign valign, int spacing);
GtkWidget *gui_create_centered_button(const char *label);
void gui_set_status(Gui *gui, GuiStatusKind kind, const char *text);
void gui_set_status_text(Gui *gui, const char *text);
void gui_show_message_dialog(Gui *gui, GtkMessageType type, GtkButtonsType buttons, const char *title,
                             const char *message);
void gui_prepare_modal_dialog(Gui *gui, GtkWidget *dialog);
void gui_destroy_endgame_dialog(Gui *gui);
int gui_confirm(Gui *gui, const char *title, const char *message);
void gui_set_error(Gui *gui, AcErrorCode code);
void gui_toggle_fullscreen(Gui *gui);
void gui_update_fullscreen_button(Gui *gui);
void gui_attach_move_provider(Gui *gui);
void gui_invalidate_async_results(Gui *gui);
void gui_cancel_async_jobs(Gui *gui);
void gui_maybe_start_ai_job(Gui *gui, const GuiView *state);
int gui_start_hint_job(Gui *gui);

int gui_current_turn_is_ai(const GuiView *state);
const char *gui_game_mode_title(AcGameMode mode);
const char *gui_game_result_text(AcGameResult result);
AcAIDifficulty gui_difficulty_from_index(int index);
int gui_difficulty_index(AcAIDifficulty difficulty);
void gui_format_elapsed_text(char buffer[32], int64_t elapsedSeconds);
void gui_format_timer_text(char buffer[32], const char *prefix, int seconds);
void gui_format_position_text(AcSquare pos, char buffer[8]);
void gui_format_hint_text(AcMove move, char buffer[64]);
const char *gui_get_piece_asset_path(AcPiece piece);
GdkPixbuf *gui_get_piece_pixbuf(AcPiece piece, int size);
GdkPixbuf *gui_get_ui_icon_pixbuf(const char *filename, int size);
GtkWidget *gui_create_ui_icon(const char *filename, int size);
void gui_set_button_icon(GtkWidget *button, const char *filename, int size);
void gui_format_piece_fallback_text(AcPiece piece, char buffer[4]);

void gui_build_main_menu(Gui *gui);
void gui_build_mode_menu(Gui *gui);
void gui_build_setup_menu(Gui *gui);
void gui_build_gameplay_ui(Gui *gui, const GuiView *state);
void gui_build_endgame_menu(Gui *gui, const GuiView *state);
void gui_build_screen_for_state(Gui *gui, const GuiView *state);

void gui_apply_pending_setup_config(Gui *gui);
int gui_collect_setup_config(Gui *gui, AcGameConfig *config, AcErrorCode *errorCode);

void gui_update_board(Gui *gui, const GuiView *state);
void gui_update_movelist(Gui *gui, const GuiView *state);
void gui_update_clock(Gui *gui);
void gui_update_timers(Gui *gui, const GuiView *state);
void gui_update_turn_display(Gui *gui, AcColor turn);
void gui_update_gameplay_controls(Gui *gui, const GuiView *state);
void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf);
void gui_clear_move_highlights(Gui *gui);
void gui_refresh_move_highlights(Gui *gui);
void gui_clear_hint_highlight(Gui *gui);
void gui_show_hint_move(Gui *gui, AcMove move);
void gui_refresh_hint_highlight(Gui *gui);

void gui_sync(Gui *gui);

const GuiView *gui_get_state(const Gui *gui);
int gui_process_events(void);
int gui_window_is_valid(const Gui *gui);

gboolean gui_on_sync_tick(gpointer user_data);
void gui_on_window_destroy(GtkWidget *widget, gpointer user_data);
void gui_on_new_game_clicked(GtkButton *button, gpointer user_data);
void gui_on_quit_game_clicked(GtkButton *button, gpointer user_data);
void gui_on_mode_selected(GtkButton *button, gpointer user_data);
void gui_on_back_clicked(GtkButton *button, gpointer user_data);
void gui_on_start_clicked(GtkButton *button, gpointer user_data);
void gui_on_setup_timer_toggled(GtkToggleButton *button, gpointer user_data);
void gui_on_submit_move_clicked(GtkButton *button, gpointer user_data);
void gui_on_move_entry_changed(GtkEditable *editable, gpointer user_data);
gboolean gui_on_board_cell_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
void gui_on_undo_clicked(GtkButton *button, gpointer user_data);
void gui_on_hint_clicked(GtkButton *button, gpointer user_data);
void gui_on_fullscreen_clicked(GtkButton *button, gpointer user_data);
void gui_on_leave_game_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_new_game_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_main_menu_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_exit_clicked(GtkButton *button, gpointer user_data);

Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);
int gui_request_new_game(Gui *gui);
int gui_request_back(Gui *gui);
int gui_request_exit(Gui *gui);
int gui_start_game(Gui *gui, const AcGameConfig *config, AcErrorCode *error);
int gui_submit_request(Gui *gui, AcMoveRequest request, AcErrorCode *error);
const char *ac_get_error_message(AcErrorCode code);
#endif
