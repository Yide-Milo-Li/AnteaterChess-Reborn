#include <assert.h>
#include <stddef.h>

#include "core/board.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/movelist.h"
#include "core/piece.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"
#include "system/controller.h"
#include "system/controller_driver.h"
#include "system/event.h"
#include "system/system_state.h"
#include "input/move_request.h"

/* Build a fresh controller using the public default configuration. */
static Controller fresh_controller(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    initController(&controller, &config);
    return controller;
}

/* Rebuild a minimal gameplay position directly on one controller-owned state. */
static void seed_empty_gameplay_position(GameState *state, Color turn) {
    int row;
    int col;

    assert(state != NULL);
    for (row = 0; row < ROWS; ++row) {
        for (col = 0; col < COLS; ++col) {
            setPiece(&state->board, createPosition(row, col),
                createPiece(EMPTY_PIECE, EMPTY_COLOR));
        }
    }

    state->currentTurn = turn;
    initMoveList(&state->moveHistory);
    state->moveCount = 0;
    state->gameOver = 0;
    state->result = RESULT_NONE;
}

/* Prepare a minimal gameplay position where one white ant has one legal move. */
static void seed_simple_ant_position(GameState *state) {
    assert(state != NULL);
    seed_empty_gameplay_position(state, WHITE);
    setPiece(&state->board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&state->board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&state->board, createPosition(6, 0), createPiece(ANT, WHITE));
}

static int first_legal_move_provider(const GameState *state, Move *move, void *context) {
    MoveList moves;
    Move *candidate;

    (void)context;
    if (state == NULL || move == NULL) {
        return 1;
    }

    if (generateLegalMoves(state, &moves) != 0 || getMoveCount(&moves) <= 0) {
        return 1;
    }

    candidate = getMove(&moves, 0);
    if (candidate == NULL) {
        return 1;
    }

    *move = *candidate;
    return 0;
}

/* Check that controller initialization resets state and queue storage. */
static void test_init_controller_resets_state_and_queue(void) {
    Controller controller = fresh_controller();

    assert(controllerGetState(NULL) == NULL);
    assert(controllerGetState(&controller) == &controller.state);
    assert(controller.state.systemState == INIT_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(isControlQueueEmpty(&controller.queue));
    assert(isSystemQueueEmpty(&controller.queue));
    assert(isGameplayQueueEmpty(&controller.queue));
}

/* Check that public enqueueing routes events into the expected priority
 * subqueues. */
static void test_controller_enqueue_event_routes_by_priority(void) {
    Controller controller = fresh_controller();

    assert(controllerEnqueueEvent(&controller, createUndoEvent()) == 0);
    assert(controllerEnqueueEvent(&controller, createSystemEvent(EVENT_TIMER_EXPIRED)) == 0);
    assert(controllerEnqueueEvent(&controller, createSystemEvent(EVENT_NEW_GAME)) == 0);

    assert(dequeueControlEvent(&controller.queue).type == EVENT_NEW_GAME);
    assert(dequeueSystemEvent(&controller.queue).type == EVENT_TIMER_EXPIRED);
    assert(dequeueGameplayEvent(&controller.queue).type == EVENT_UNDO);
}

/* Check that one controller tick bootstraps INIT into the main menu with the
 * compatibility EVENT_NONE handshake. */
static void test_controller_tick_bootstraps_init_state(void) {
    Controller controller = fresh_controller();
    Event processedEvent;

    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(processedEvent.type == EVENT_NONE);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that one queued external event is processed in a single tick. */
static void test_controller_tick_processes_one_queued_event(void) {
    Controller controller = fresh_controller();
    Event processedEvent;

    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
    assert(controllerEnqueueEvent(&controller, createSystemEvent(EVENT_NEW_GAME)) == 0);
    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(processedEvent.type == EVENT_NEW_GAME);
    assert(controller.state.systemState == GAME_MODE_SELECTION_STATE);
}

/* Check that ticking an already idle controller returns EVENT_NONE without
 * mutating state. */
static void test_controller_tick_returns_none_when_idle(void) {
    Controller controller = fresh_controller();
    Event processedEvent;

    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
    assert(controllerTick(&controller, &processedEvent) == 0);
    assert(processedEvent.type == EVENT_NONE);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that run-until-idle advances the boot handshake and then stops at the
 * first UI-facing idle point. */
static void test_controller_run_until_idle_bootstraps_init_state(void) {
    Controller controller = fresh_controller();

    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that the frontend sync facade drains controller-owned lifecycle work
 * without exposing driver APIs to GUI code. */
static void test_controller_sync_bootstraps_init_state(void) {
    Controller controller = fresh_controller();

    assert(controllerSync(&controller) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that run-until-idle completes the termination handshake into the
 * end-game menu. */
static void test_controller_run_until_idle_completes_termination_handshake(void) {
    Controller controller = fresh_controller();

    controller.state.systemState = GAME_TERMINATION_STATE;
    controller.state.gameOver = 1;
    controller.state.result = RESULT_TERMINATED_BY_USER;
    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == END_GAME_MENU_STATE);
}

/* Check that configured-game startup reaches gameplay without auto-playing the
 * opening AI turn. */
static void test_controller_start_configured_game_enters_gameplay(void) {
    Controller controller;
    GameConfig config;
    const GameState *state;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    state = controllerGetState(&controller);
    assert(state != NULL);
    assert(state->systemState == GAMEPLAY_STATE);
    assert(state->players[WHITE].type == AI);
    assert(state->players[BLACK].type == HUMAN);
    assert(state->currentTurn == WHITE);
    assert(state->moveHistory.count == 0);
}

/* Check that gameplay background work is controller-owned: run-until-idle
 * schedules and applies the opening AI move after configured startup. */
static void test_controller_run_until_idle_auto_plays_ai_turn(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.players[WHITE].type == AI);
    assert(controller.state.moveHistory.count == 0);

    controllerSetMoveProvider(&controller, first_legal_move_provider, NULL);
    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that AI-controlled turns stay idle unless the integration layer
 * explicitly installs a move provider. */
static void test_controller_run_until_idle_leaves_ai_turn_idle_without_provider(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.players[WHITE].type == AI);
    assert(controller.state.moveHistory.count == 0);

    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that GUI-style AI submission applies exactly one legal AI move through
 * the controller/FSM path. */
static void test_controller_submit_ai_move_applies_one_move(void) {
    Controller controller;
    GameConfig config;
    Move move;
    ErrorCode errorCode;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(first_legal_move_provider(&controller.state, &move, NULL) == 0);

    errorCode = ERR_FATAL;
    assert(controllerSubmitAIMoveDetailed(&controller, move, &errorCode) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that AI submission cannot be used to move during a human turn. */
static void test_controller_submit_ai_move_rejects_human_turn(void) {
    Controller controller;
    GameConfig config;
    Move move;
    ErrorCode errorCode;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(first_legal_move_provider(&controller.state, &move, NULL) == 0);

    errorCode = ERR_FATAL;
    assert(controllerSubmitAIMoveDetailed(&controller, move, &errorCode) != 0);
    assert(errorCode == ERR_NOT_YOUR_TURN);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
}

/* Check that an AI provider returning an illegal move fails without mutating
 * the game state. */
static void test_controller_submit_ai_move_rejects_illegal_move(void) {
    Controller controller;
    GameConfig config;
    Move move;
    ErrorCode errorCode;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    move = createMove(createPosition(0, 1), createPosition(2, 2), createPiece(KNIGHT, BLACK));

    errorCode = ERR_FATAL;
    assert(controllerSubmitAIMoveDetailed(&controller, move, &errorCode) != 0);
    assert(errorCode == ERR_ILLEGAL_MOVE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
    assert(getPiece(&controller.state.board, createPosition(0, 1)).type == KNIGHT);
}

/* Check that computer-vs-computer GUI integration can advance one AI move at a
 * time instead of draining the whole AI game in one sync call. */
static void test_controller_submit_ai_move_cvc_advances_one_step(void) {
    Controller controller;
    GameConfig config;
    Move move;
    ErrorCode errorCode;

    initDefaultGameConfig(&config);
    config.mode = MODE_COMPUTER_VS_COMPUTER;
    config.playerColor = EMPTY_COLOR;
    config.aiDifficultyWhite = DIFFICULTY_EASY;
    config.aiDifficultyBlack = DIFFICULTY_EASY;

    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(first_legal_move_provider(&controller.state, &move, NULL) == 0);

    errorCode = ERR_FATAL;
    assert(controllerSubmitAIMoveDetailed(&controller, move, &errorCode) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that the facade-style new-game request bootstraps INIT before
 * advancing to the next UI-facing menu. */
static void test_controller_request_new_game_advances_to_mode_menu(void) {
    Controller controller = fresh_controller();

    assert(controllerRequestNewGame(&controller) == 0);
    assert(controller.state.systemState == GAME_MODE_SELECTION_STATE);
}

/* Check that the facade-style back request drains the transition to the main
 * menu. */
static void test_controller_request_back_returns_to_main_menu(void) {
    Controller controller = fresh_controller();

    assert(controllerRequestNewGame(&controller) == 0);
    assert(controllerRequestBack(&controller) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);
}

/* Check that the facade-style exit request reaches EXIT without exposing queue
 * details to the caller. */
static void test_controller_request_exit_reaches_exit_state(void) {
    Controller controller = fresh_controller();

    assert(controllerRequestExit(&controller) == 0);
    assert(controller.state.systemState == EXIT_STATE);
}

/* Check that the facade move-submission helper applies one valid move and
 * drains follow-up controller work. */
static void test_controller_submit_move_applies_valid_move(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(createMoveRequest(&request, createPosition(6, 0),
        createPosition(5, 0), PROMOTION_CHOICE_NONE) == 0);
    assert(controllerSubmitMoveRequest(&controller, request) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == EMPTY_PIECE);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == ANT);
    assert(controller.state.currentTurn == BLACK);
    assert(controller.state.moveHistory.count == 1);
}

/* Check that detailed move submission reports one stable user-facing error for
 * an illegal destination instead of requiring callers to run the resolver. */
static void test_controller_submit_move_request_detailed_reports_illegal_move(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;
    ErrorCode errorCode;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(createMoveRequest(&request, createPosition(6, 0),
        createPosition(5, 1), PROMOTION_CHOICE_NONE) == 0);

    errorCode = ERR_FATAL;
    assert(controllerSubmitMoveRequestDetailed(&controller, request, &errorCode) != 0);
    assert(errorCode == ERR_ILLEGAL_MOVE);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that detailed move submission reports unavailable actions when gameplay
 * has not started yet. */
static void test_controller_submit_move_request_detailed_reports_non_gameplay(void) {
    Controller controller = fresh_controller();
    MoveRequest request;
    ErrorCode errorCode;

    assert(createMoveRequest(&request, createPosition(6, 0),
        createPosition(5, 0), PROMOTION_CHOICE_NONE) == 0);
    assert(controllerRunUntilIdle(&controller) == 0);
    assert(controller.state.systemState == MAIN_MENU_STATE);

    errorCode = ERR_FATAL;
    assert(controllerSubmitMoveRequestDetailed(&controller, request, &errorCode) != 0);
    assert(errorCode == ERR_ACTION_UNAVAILABLE);
}

/* Check that human-facing submit helpers do not let frontends play an AI turn. */
static void test_controller_submit_move_request_detailed_rejects_ai_turn(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;
    ErrorCode errorCode;

    initDefaultGameConfig(&config);
    config.mode = MODE_HUMAN_VS_COMPUTER;
    config.playerColor = BLACK;
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    assert(controller.state.systemState == GAMEPLAY_STATE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.players[WHITE].type == AI);
    assert(createMoveRequest(&request, createPosition(7, 1),
        createPosition(5, 2), PROMOTION_CHOICE_NONE) == 0);

    errorCode = ERR_FATAL;
    assert(controllerSubmitMoveRequestDetailed(&controller, request, &errorCode) != 0);
    assert(errorCode == ERR_NOT_YOUR_TURN);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that a legal submitted move at history capacity becomes a draw through
 * the public controller path. */
static void test_controller_submit_move_draws_at_history_capacity(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    initBoard(&controller.state.board);
    controller.state.currentTurn = WHITE;
    controller.state.moveHistory.count = MAX_MOVES;
    controller.state.moveCount = MAX_MOVES;

    assert(createMoveRequest(&request, createPosition(7, 1),
        createPosition(5, 2), PROMOTION_CHOICE_NONE) == 0);
    assert(controllerSubmitMoveRequest(&controller, request) == 0);
    assert(controller.state.systemState == END_GAME_MENU_STATE);
    assert(controller.state.result == RESULT_DRAW);
    assert(getPiece(&controller.state.board, createPosition(7, 1)).type == KNIGHT);
    assert(getPiece(&controller.state.board, createPosition(5, 2)).type == EMPTY_PIECE);
}

/* Check that the richer facade move request applies an explicit promotion
 * choice without exposing arbitrary Move construction to callers. */
static void test_controller_submit_move_request_applies_promotion_choice(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;
    ErrorCode errorCode;
    int needsPromotion;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_empty_gameplay_position(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(1, 2), createPiece(ANT, WHITE));

    assert(createMoveRequest(&request, createPosition(1, 2),
        createPosition(0, 2), PROMOTION_CHOICE_ROOK) == 0);
    needsPromotion = 0;
    assert(controllerMoveRequestNeedsPromotion(&controller, request, &needsPromotion) == 0);
    assert(needsPromotion == 1);

    errorCode = ERR_FATAL;
    assert(controllerSubmitMoveRequestDetailed(&controller, request, &errorCode) == 0);
    assert(getPiece(&controller.state.board, createPosition(0, 2)).type == ROOK);
    assert(getPiece(&controller.state.board, createPosition(0, 2)).color == WHITE);
}

/* Check that GUI-facing request submission can still choose queen promotion
 * without relying on the removed legacy Command compatibility path. */
static void test_controller_submit_move_request_promotes_to_queen(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_empty_gameplay_position(&controller.state, WHITE);
    setPiece(&controller.state.board, createPosition(7, 5), createPiece(KING, WHITE));
    setPiece(&controller.state.board, createPosition(0, 5), createPiece(KING, BLACK));
    setPiece(&controller.state.board, createPosition(1, 2), createPiece(ANT, WHITE));

    assert(createMoveRequest(&request, createPosition(1, 2),
        createPosition(0, 2), PROMOTION_CHOICE_QUEEN) == 0);
    assert(controllerSubmitMoveRequest(&controller, request) == 0);
    assert(getPiece(&controller.state.board, createPosition(0, 2)).type == QUEEN);
    assert(getPiece(&controller.state.board, createPosition(0, 2)).color == WHITE);
}

/* Check that the facade undo helper rewinds back to the previous human turn
 * and leaves the controller idle again. */
static void test_controller_request_undo_restores_position(void) {
    Controller controller;
    GameConfig config;
    MoveRequest request;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(createMoveRequest(&request, createPosition(6, 0),
        createPosition(5, 0), PROMOTION_CHOICE_NONE) == 0);
    assert(controllerSubmitMoveRequest(&controller, request) == 0);
    assert(controllerRequestUndo(&controller) == 0);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type == ANT);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type == EMPTY_PIECE);
    assert(controller.state.currentTurn == WHITE);
    assert(controller.state.moveHistory.count == 0);
}

/* Check that the facade leave-game helper drains the termination handshake
 * into the end-game menu. */
static void test_controller_request_leave_game_reaches_endgame_menu(void) {
    Controller controller;
    GameConfig config;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    assert(controllerRequestLeaveGame(&controller) == 0);
    assert(controller.state.systemState == END_GAME_MENU_STATE);
    assert(controller.state.gameOver == 1);
    assert(controller.state.result == RESULT_TERMINATED_BY_USER);
}

/* Check that the facade hint helper returns one legal move without mutating
 * controller-owned gameplay state. */
static void test_controller_get_hint_returns_move_without_mutating_state(void) {
    Controller controller;
    GameConfig config;
    GameState before;
    Move move;

    initDefaultGameConfig(&config);
    assert(controllerStartConfiguredGame(&controller, &config) == 0);
    seed_simple_ant_position(&controller.state);
    before = controller.state;

    assert(controllerGetHint(&controller, &move) == 0);
    assert(validateMove(&controller.state, move) == 1);
    assert(getPiece(&controller.state.board, createPosition(6, 0)).type
        == getPiece(&before.board, createPosition(6, 0)).type);
    assert(getPiece(&controller.state.board, createPosition(5, 0)).type
        == getPiece(&before.board, createPosition(5, 0)).type);
    assert(controller.state.currentTurn == before.currentTurn);
    assert(controller.state.moveHistory.count == before.moveHistory.count);
}

/* Run the controller regression suite. */
int main(void) {
    test_init_controller_resets_state_and_queue();
    test_controller_enqueue_event_routes_by_priority();
    test_controller_tick_bootstraps_init_state();
    test_controller_tick_processes_one_queued_event();
    test_controller_tick_returns_none_when_idle();
    test_controller_run_until_idle_bootstraps_init_state();
    test_controller_sync_bootstraps_init_state();
    test_controller_run_until_idle_completes_termination_handshake();
    test_controller_start_configured_game_enters_gameplay();
    test_controller_run_until_idle_auto_plays_ai_turn();
    test_controller_run_until_idle_leaves_ai_turn_idle_without_provider();
    test_controller_submit_ai_move_applies_one_move();
    test_controller_submit_ai_move_rejects_human_turn();
    test_controller_submit_ai_move_rejects_illegal_move();
    test_controller_submit_ai_move_cvc_advances_one_step();
    test_controller_request_new_game_advances_to_mode_menu();
    test_controller_request_back_returns_to_main_menu();
    test_controller_request_exit_reaches_exit_state();
    test_controller_submit_move_applies_valid_move();
    test_controller_submit_move_request_detailed_reports_illegal_move();
    test_controller_submit_move_request_detailed_reports_non_gameplay();
    test_controller_submit_move_request_detailed_rejects_ai_turn();
    test_controller_submit_move_draws_at_history_capacity();
    test_controller_submit_move_request_applies_promotion_choice();
    test_controller_submit_move_request_promotes_to_queen();
    test_controller_request_undo_restores_position();
    test_controller_request_leave_game_reaches_endgame_menu();
    test_controller_get_hint_returns_move_without_mutating_state();
    return 0;
}
