#ifndef CHESS_UI_GUI_INTERNAL_H
#define CHESS_UI_GUI_INTERNAL_H

#include <stdint.h>
#include <gtk/gtk.h>

#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "error/error_code.h"
#include "system/controller.h"
#include "ui/gui.h"

typedef struct GuiAsyncJob GuiAsyncJob;

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
    GtkWidget *black_timer_label;
    GtkWidget *white_timer_label;
    GtkWidget *status_label;
    GtkWidget *from_entry;
    GtkWidget *to_entry;
    GtkWidget *submit_button;
    GtkWidget *undo_button;
    GtkWidget *hint_button;
    GtkWidget *setup_timer_toggle;
    GtkWidget *setup_hours_spin;
    GtkWidget *setup_minutes_spin;
    GtkWidget *setup_seconds_spin;
    GtkWidget *setup_timer_widgets[6];
    GtkWidget *setup_side_white;
    GtkWidget *setup_side_black;
    GtkWidget *setup_ai_diff_buttons[3];
    GtkWidget *setup_white_diff_buttons[3];
    GtkWidget *setup_black_diff_buttons[3];
    GuiMoveProvider move_provider;
    void *move_provider_context;
    GuiHintProvider hint_provider;
    void *hint_provider_context;
    GuiAsyncJob *ai_job;
    GuiAsyncJob *hint_job;
    Controller controller;
    GameConfig pendingConfig;
    SystemState last_rendered_state;
    Color last_turn;
    int last_move_count;
    unsigned int async_generation;
    int ai_failure_active;
    int ai_failure_move_count;
    Color ai_failure_turn;
    int highlight_destinations[8][10];
    Position highlight_from;
    guint sync_source_id;
    int has_rendered_state;
    int has_highlight_from;
    int endgame_dialog_shown;
    int should_quit;
};

void gui_clear_view_refs(Gui *gui);
void gui_rebuild_root_box(Gui *gui, GtkAlign halign, GtkAlign valign, int spacing);
GtkWidget *gui_create_centered_button(const char *label);
void gui_set_status_text(Gui *gui, const char *text);
void gui_show_message_dialog(Gui *gui, GtkMessageType type,
                             GtkButtonsType buttons,
                             const char *title,
                             const char *message);
int gui_confirm(Gui *gui, const char *title, const char *message);
void gui_set_error(Gui *gui, ErrorCode code);
void gui_attach_move_provider(Gui *gui);
void gui_invalidate_async_results(Gui *gui);
void gui_cancel_async_jobs(Gui *gui);
void gui_maybe_start_ai_job(Gui *gui, const GameState *state);
int gui_start_hint_job(Gui *gui);

int gui_current_turn_is_ai(const GameState *state);
const char *gui_game_mode_title(GameMode mode);
const char *gui_game_result_text(GameResult result);
AIDifficulty gui_difficulty_from_index(int index);
int gui_difficulty_index(AIDifficulty difficulty);
void gui_format_elapsed_text(char buffer[32], int64_t elapsedSeconds);
void gui_format_timer_text(char buffer[32], const char *prefix, int seconds);
void gui_format_position_text(Position pos, char buffer[8]);
void gui_format_hint_text(Move move, char buffer[64]);
const char *gui_get_piece_asset_path(Piece piece);
GdkPixbuf *gui_get_piece_pixbuf(Piece piece, int size);
GdkPixbuf *gui_get_ui_icon_pixbuf(const char *filename, int size);
GtkWidget *gui_create_ui_icon(const char *filename, int size);
void gui_set_button_icon(GtkWidget *button, const char *filename, int size);
void gui_format_piece_fallback_text(Piece piece, char buffer[4]);

void gui_build_main_menu(Gui *gui);
void gui_build_mode_menu(Gui *gui);
void gui_build_setup_menu(Gui *gui);
void gui_build_gameplay_ui(Gui *gui, const GameState *state);
void gui_build_endgame_menu(Gui *gui, const GameState *state);
void gui_build_screen_for_state(Gui *gui, const GameState *state);

void gui_apply_pending_setup_config(Gui *gui);
int gui_collect_setup_config(Gui *gui, GameConfig *config, ErrorCode *errorCode);

void gui_update_board(Gui *gui, const GameState *state);
void gui_update_movelist(Gui *gui, const GameState *state);
void gui_update_clock(Gui *gui);
void gui_update_timers(Gui *gui, const GameState *state);
void gui_update_turn_display(Gui *gui, Color turn);
void gui_update_gameplay_controls(Gui *gui, const GameState *state);
void gui_set_board_image(Gui *gui, int row, int col, GdkPixbuf *pixbuf);
void gui_clear_move_highlights(Gui *gui);
void gui_refresh_move_highlights(Gui *gui);

void gui_sync_from_controller(Gui *gui);
int gui_render_snapshot(Gui *gui, const GameState *state);
const GameState *gui_get_state(const Gui *gui);
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
void gui_on_leave_game_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_new_game_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_main_menu_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_exit_clicked(GtkButton *button, gpointer user_data);
void gui_on_endgame_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data);
gboolean gui_on_async_job_finished(gpointer user_data);

#endif
