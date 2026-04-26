#include "gui_internal.h"

#include <ctype.h>

#include "core/movelist.h"
#include "core/position.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"
#include "input/move_request_parser.h"

static int gui_text_is_blank(const char *text) {
    const unsigned char *cursor = (const unsigned char *)text;

    if (cursor == NULL) {
        return 1;
    }

    while (*cursor != '\0') {
        if (!isspace(*cursor)) {
            return 0;
        }
        ++cursor;
    }

    return 1;
}

static void gui_set_style_class(GtkWidget *widget, const char *className, int enabled) {
    GtkStyleContext *context;

    if (!GTK_IS_WIDGET(widget) || className == NULL) {
        return;
    }

    context = gtk_widget_get_style_context(widget);
    if (enabled) {
        gtk_style_context_add_class(context, className);
    } else {
        gtk_style_context_remove_class(context, className);
    }
}

static void gui_mark_entry(GtkWidget *entry, int state) {
    if (!GTK_IS_WIDGET(entry)) {
        return;
    }

    gui_set_style_class(entry, "move-input-valid", 0);
    gui_set_style_class(entry, "move-input-invalid", 0);
    if (state > 0) {
        gui_set_style_class(entry, "move-input-valid", 1);
    } else if (state < 0) {
        gui_set_style_class(entry, "move-input-invalid", 1);
    }
}

static int gui_destination_is_highlighted(const Gui *gui, Position pos) {
    if (gui == NULL || !isValidPosition(pos)) {
        return 0;
    }

    return gui->highlight_destinations[pos.row][pos.col] != 0;
}

void gui_clear_move_highlights(Gui *gui) {
    int row;
    int col;

    if (gui == NULL) {
        return;
    }

    gui->has_highlight_from = 0;
    gui->highlight_from = createPosition(-1, -1);
    for (row = 0; row < 8; ++row) {
        for (col = 0; col < 10; ++col) {
            gui->highlight_destinations[row][col] = 0;
            if (!GTK_IS_WIDGET(gui->board_cells[row][col])) {
                continue;
            }
            gui_set_style_class(gui->board_cells[row][col], "highlight-from", 0);
            gui_set_style_class(gui->board_cells[row][col], "highlight-destination", 0);
            gui_set_style_class(gui->board_cells[row][col], "highlight-selected", 0);
        }
    }

    gui_mark_entry(gui->from_entry, 0);
    gui_mark_entry(gui->to_entry, 0);
}

void gui_refresh_move_highlights(Gui *gui) {
    const GameState *state;
    const char *fromText;
    const char *toText;
    Position from;
    Position to;
    MoveList *moves;
    int index;

    if (gui == NULL) {
        return;
    }

    gui_clear_move_highlights(gui);
    state = gui_get_state(gui);
    if (state == NULL || state->systemState != GAMEPLAY_STATE
        || gui_current_turn_is_ai(state)
        || !GTK_IS_ENTRY(gui->from_entry)
        || !GTK_IS_ENTRY(gui->to_entry)) {
        return;
    }

    fromText = gtk_entry_get_text(GTK_ENTRY(gui->from_entry));
    toText = gtk_entry_get_text(GTK_ENTRY(gui->to_entry));
    if (gui_text_is_blank(fromText)) {
        if (!gui_text_is_blank(toText)) {
            gui_mark_entry(gui->to_entry, -1);
        }
        return;
    }

    from = parsePosition(fromText);
    if (!isValidPosition(from) || validateSelection(state, from) != SELECT_VALID) {
        gui_mark_entry(gui->from_entry, -1);
        if (!gui_text_is_blank(toText)) {
            gui_mark_entry(gui->to_entry, -1);
        }
        return;
    }

    gui_mark_entry(gui->from_entry, 1);
    gui->has_highlight_from = 1;
    gui->highlight_from = from;
    gui_set_style_class(gui->board_cells[from.row][from.col], "highlight-from", 1);

    moves = g_new0(MoveList, 1);
    if (moves == NULL) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    if (generateLegalMovesForPosition(state, from, moves) == 0) {
        for (index = 0; index < getMoveCount(moves); ++index) {
            Move *move = getMove(moves, index);

            if (move == NULL || !isValidPosition(move->to)) {
                continue;
            }

            gui->highlight_destinations[move->to.row][move->to.col] = 1;
            gui_set_style_class(gui->board_cells[move->to.row][move->to.col],
                "highlight-destination",
                1);
        }
    }
    g_free(moves);

    if (gui_text_is_blank(toText)) {
        return;
    }

    to = parsePosition(toText);
    if (isValidPosition(to) && gui_destination_is_highlighted(gui, to)) {
        gui_mark_entry(gui->to_entry, 1);
        gui_set_style_class(gui->board_cells[to.row][to.col], "highlight-selected", 1);
    } else {
        gui_mark_entry(gui->to_entry, -1);
    }
}

static int gui_select_promotion_choice(Gui *gui, PromotionChoice *choice) {
    GtkWidget *dialog;
    GtkWindow *parent = NULL;
    int response;

    if (choice == NULL) {
        return 1;
    }

    if (gui_window_is_valid(gui)) {
        parent = GTK_WINDOW(gui->window);
    }

    dialog = gtk_message_dialog_new(parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_NONE,
        "%s",
        "Choose promotion piece");
    gtk_window_set_title(GTK_WINDOW(dialog), "Promotion");
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Queen", PROMOTION_CHOICE_QUEEN);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Rook", PROMOTION_CHOICE_ROOK);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Bishop", PROMOTION_CHOICE_BISHOP);
    gtk_dialog_add_button(GTK_DIALOG(dialog), "Knight", PROMOTION_CHOICE_KNIGHT);

    response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    switch (response) {
        case PROMOTION_CHOICE_QUEEN:
        case PROMOTION_CHOICE_ROOK:
        case PROMOTION_CHOICE_BISHOP:
        case PROMOTION_CHOICE_KNIGHT:
            *choice = (PromotionChoice)response;
            return 0;
        default:
            return 1;
    }
}

void gui_on_new_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    gui_invalidate_async_results(gui);
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_quit_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (!gui_confirm(gui, "Quit Game", "Are you sure you want to quit?")) {
        return;
    }

    gui_invalidate_async_results(gui);
    if (controllerRequestExit(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_mode_selected(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    GameMode mode = (GameMode) GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "game-mode"));

    initGameConfigForMode(&gui->pendingConfig, mode);
    gui_invalidate_async_results(gui);
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_back_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    gui_invalidate_async_results(gui);
    if (controllerRequestBack(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_start_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    GameConfig config;
    ErrorCode errorCode;

    (void)button;
    if (gui_collect_setup_config(gui, &config, &errorCode) != 0) {
        gui_set_error(gui, errorCode);
        return;
    }

    gui_invalidate_async_results(gui);
    if (controllerStartConfiguredGame(&gui->controller, &config) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_attach_move_provider(gui);
    gui->pendingConfig = config;
    gui_sync_from_controller(gui);
}

void gui_on_setup_timer_toggled(GtkToggleButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    gboolean active;
    int index;

    (void)button;
    if (gui == NULL) {
        return;
    }

    active = GTK_IS_TOGGLE_BUTTON(gui->setup_timer_toggle)
        ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui->setup_timer_toggle))
        : FALSE;

    for (index = 0; index < 6; ++index) {
        if (!GTK_IS_WIDGET(gui->setup_timer_widgets[index])) {
            continue;
        }

        if (active) {
            gtk_widget_show(gui->setup_timer_widgets[index]);
        } else {
            gtk_widget_hide(gui->setup_timer_widgets[index]);
        }
    }
}

void gui_on_submit_move_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    const char *fromText;
    const char *toText;
    MoveRequest request;
    ErrorCode errorCode;
    int needsPromotion;

    (void)button;
    if (gui == NULL || !GTK_IS_ENTRY(gui->from_entry) || !GTK_IS_ENTRY(gui->to_entry)) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    fromText = gtk_entry_get_text(GTK_ENTRY(gui->from_entry));
    toText = gtk_entry_get_text(GTK_ENTRY(gui->to_entry));
    if (parseMoveRequestFields(fromText, toText,
        PROMOTION_CHOICE_QUEEN, &request) != 0) {
        gui_set_error(gui, ERR_INVALID_MOVE_FORMAT);
        return;
    }

    needsPromotion = 0;
    if (controllerMoveRequestNeedsPromotion(&gui->controller, request, &needsPromotion) == 0
        && needsPromotion) {
        PromotionChoice promotion;

        if (gui_select_promotion_choice(gui, &promotion) != 0) {
            return;
        }
        request.promotion = promotion;
    }

    errorCode = ERR_ILLEGAL_MOVE;
    gui_invalidate_async_results(gui);
    if (controllerSubmitMoveRequestDetailed(&gui->controller, request, &errorCode) != 0) {
        gui_set_error(gui, errorCode);
        return;
    }

    gtk_entry_set_text(GTK_ENTRY(gui->from_entry), "");
    gtk_entry_set_text(GTK_ENTRY(gui->to_entry), "");
    gui_set_status_text(gui, "");
    gui_clear_move_highlights(gui);
    gui_sync_from_controller(gui);
}

void gui_on_move_entry_changed(GtkEditable *editable, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)editable;
    gui_refresh_move_highlights(gui);
}

gboolean gui_on_board_cell_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    const GameState *state;
    Position pos;
    char positionText[8];
    int row;
    int col;

    if (gui == NULL || event == NULL) {
        return FALSE;
    }

    state = gui_get_state(gui);
    if (state == NULL || state->systemState != GAMEPLAY_STATE) {
        return FALSE;
    }

    if (gui_current_turn_is_ai(state)) {
        gui_set_error(gui, ERR_NOT_YOUR_TURN);
        return TRUE;
    }

    row = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "board-row"));
    col = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "board-col"));
    pos = createPosition(row, col);
    if (!isValidPosition(pos) || !GTK_IS_ENTRY(gui->from_entry) || !GTK_IS_ENTRY(gui->to_entry)) {
        return FALSE;
    }

    gui_format_position_text(pos, positionText);
    if (event->button == 1) {
        gtk_entry_set_text(GTK_ENTRY(gui->from_entry), positionText);
        gtk_entry_set_text(GTK_ENTRY(gui->to_entry), "");
        gui_set_status_text(gui, "");
        gui_refresh_move_highlights(gui);
        return TRUE;
    }

    if (event->button == 3) {
        gtk_entry_set_text(GTK_ENTRY(gui->to_entry), positionText);
        gui_refresh_move_highlights(gui);
        if (gui_destination_is_highlighted(gui, pos)) {
            gui_on_submit_move_clicked(NULL, gui);
        } else {
            gui_set_error(gui, ERR_ILLEGAL_MOVE);
        }
        return TRUE;
    }

    return FALSE;
}

void gui_on_undo_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    gui_invalidate_async_results(gui);
    if (controllerRequestUndo(&gui->controller) != 0) {
        gui_set_error(gui, ERR_UNDO_UNAVAILABLE);
        return;
    }

    gui_set_status_text(gui, "");
    gui_sync_from_controller(gui);
}

void gui_on_hint_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    Move move;
    char hintText[64];
    const GameState *state;
    int result;

    (void)button;
    if (gui == NULL) {
        return;
    }

    if (gui_start_hint_job(gui) == 0) {
        return;
    }

    state = gui_get_state(gui);
    if (gui != NULL && gui->hint_provider != NULL) {
        gui_set_error(gui, ERR_HINT_UNAVAILABLE);
        return;
    } else {
        result = controllerGetHint(&gui->controller, &move);
    }

    if (result != 0) {
        gui_set_error(gui, ERR_HINT_UNAVAILABLE);
        return;
    }

    gui_format_hint_text(move, hintText);
    gui_set_status_text(gui, hintText);
}

void gui_on_leave_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    if (!gui_confirm(gui, "Leave Game", "Leave the current game?")) {
        return;
    }

    gui_invalidate_async_results(gui);
    if (controllerRequestLeaveGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_new_game_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    gui_invalidate_async_results(gui);
    if (controllerRequestNewGame(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_main_menu_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    gui_invalidate_async_results(gui);
    if (controllerRequestBack(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_exit_clicked(GtkButton *button, gpointer user_data) {
    Gui *gui = (Gui *) user_data;

    (void)button;
    gui_invalidate_async_results(gui);
    if (controllerRequestExit(&gui->controller) != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}

void gui_on_endgame_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    Gui *gui = (Gui *) user_data;
    int result;

    if (gui == NULL) {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        return;
    }

    gui->endgame_dialog_shown = 0;
    gtk_widget_destroy(GTK_WIDGET(dialog));

    if (response_id == GTK_RESPONSE_ACCEPT) {
        gui_invalidate_async_results(gui);
        result = controllerRequestNewGame(&gui->controller);
    } else if (response_id == GTK_RESPONSE_APPLY) {
        gui_invalidate_async_results(gui);
        result = controllerRequestBack(&gui->controller);
    } else {
        gui_invalidate_async_results(gui);
        result = controllerRequestExit(&gui->controller);
    }

    if (result != 0) {
        gui_set_error(gui, ERR_FATAL);
        return;
    }

    gui_sync_from_controller(gui);
}
