#include "cli/cli_app.h"

#include <stdio.h>
#include <stddef.h>

#include "cli/cli_feedback.h"
#include "cli/cli_gameplay.h"
#include "cli/cli_menu.h"
#include "cli/cli_renderer.h"
#include "core/gameconfig.h"
#include "core/gamestate.h"
#include "core/board.h"
#include "input/input.h"
#include "core/position.h"
#include "gameplay/movegen.h"
#include "gameplay/validation.h"
#include "system/controller.h"
#include "system/event.h"
#include "turn/turn_timer.h"

/*
 * Alignment assumptions for future extensions:
 * - CLI headers are the truth source for the text-front-end contract in this file.
 * - This file remains a temporary text frontend, but it now drives runtime
 *   flow through controller.h instead of orchestrating its own event queues.
 * - Controller owns event routing and internal work; gameplay and AI modules
 *   remain the source of truth for rules and move selection.
 */

#define ANSI_RESET  "\x1b[0m"
#define ANSI_ACCENT "\x1b[1;36m"

/* Print one lightweight gameplay page header before the status panel and board. */
static void print_gameplay_page_header(void) {
    printf("\n%s================== Anteater Chess CLI ==================%s\n",
        ANSI_ACCENT, ANSI_RESET);
    printf("%sGameplay View%s\n", ANSI_ACCENT, ANSI_RESET);
}

/* Render the shared gameplay view before human or AI turn handling. */
static int render_gameplay_view(const GameState *state) {
    print_gameplay_page_header();
    if (cliDisplayTurn(state->currentTurn) != 0
        || cliDisplayGameStatus(state) != 0
        || cliRenderBoard(state) != 0) {
        return 1;
    }

    return 0;
}

/* Report whether the current side to move is controlled by the AI. */
static int current_turn_is_ai(const GameState *state) {
    if (state == NULL || state->currentTurn < WHITE || state->currentTurn > BLACK) {
        return 0;
    }

    return state->players[state->currentTurn].type == AI;
}

/* Check whether the controller currently has pending queued work. */
static int controller_queue_is_empty(const Controller *controller) {
    if (controller == NULL) {
        return 1;
    }

    return isControlQueueEmpty(&controller->queue)
        && isSystemQueueEmpty(&controller->queue)
        && isGameplayQueueEmpty(&controller->queue);
}

/* Return the shared color label used in setup and gameplay summaries. */
static const char *color_label(Color color) {
    return (color == BLACK) ? "Black" : "White";
}

/* Return the shared piece label used in move summaries. */
static const char *piece_label(PieceType type) {
    switch (type) {
        case ANT:
            return "Ant";
        case ROOK:
            return "Rook";
        case KNIGHT:
            return "Knight";
        case BISHOP:
            return "Bishop";
        case QUEEN:
            return "Queen";
        case KING:
            return "King";
        case ANTEATER:
            return "Anteater";
        case EMPTY_PIECE:
        default:
            return "Piece";
    }
}

/* Convert one valid board coordinate into the CLI algebraic display format. */
static void format_position_text(Position pos, char buffer[8]) {
    int displayRow;

    if (buffer == NULL) {
        return;
    }

    if (!isValidPosition(pos)) {
        snprintf(buffer, 8, "??");
        return;
    }

    displayRow = 8 - pos.row;
    buffer[0] = (char)('A' + pos.col);
    buffer[1] = (char)('0' + displayRow);
    buffer[2] = '\0';
}

/* Return the optional special-move label for one public move type. */
static const char *special_move_label(SpecialMove type) {
    switch (type) {
        case PROMOTION_QUEEN:
            return "promotion to queen";
        case PROMOTION_ROOK:
            return "promotion to rook";
        case PROMOTION_BISHOP:
            return "promotion to bishop";
        case PROMOTION_KNIGHT:
            return "promotion to knight";
        case EN_PASSANT:
            return "en passant";
        case CASTLING_KINGSIDE:
            return "castle kingside";
        case CASTLING_QUEENSIDE:
            return "castle queenside";
        case ANTEATER_CAPTURE:
            return "anteater chain";
        case NO_SPECIAL_MOVE:
        default:
            return NULL;
    }
}

/* Report one non-terminal turn-timer timeout as a skipped turn, not a game
 * result. */
static void print_timeout_skip_message(Color expiredSide) {
    Color nextSide;

    nextSide = (expiredSide == BLACK) ? WHITE : BLACK;
    printf("%s[Timer]%s %s timed out; turn passed to %s.\n",
        ANSI_ACCENT,
        ANSI_RESET,
        color_label(expiredSide),
        color_label(nextSide));
}

/* Classify one rejected move command into the most precise current public
 * error code. */
static ErrorCode classify_move_error(const GameState *state, Command command);

/* Promotion ambiguity is intentionally normalized to queen for the human CLI
 * because the public command interface still carries only from/to. */
static int is_promotion_move(SpecialMove type) {
    return type == PROMOTION_QUEEN
        || type == PROMOTION_ROOK
        || type == PROMOTION_BISHOP
        || type == PROMOTION_KNIGHT;
}

/* Resolve one parsed CLI move command against the current legal move list. */
static int resolve_cli_move_command(const GameState *state, Command command, Move *resolvedMove) {
    MoveList candidates;
    Piece movingPiece;
    Move *promotionQueenCandidate;
    int matchingCount;
    int allMatchesArePromotions;
    int index;

    if (state == NULL || resolvedMove == NULL || command.type != CMD_MOVE) {
        return 1;
    }

    if (validateSelection(state, command.from) != SELECT_VALID || !isValidPosition(command.to)) {
        return 1;
    }

    movingPiece = getPiece(&state->board, command.from);
    if (movingPiece.type == EMPTY_PIECE) {
        return 1;
    }

    if (generateLegalMovesForPosition(state, command.from, &candidates) != 0) {
        return 1;
    }

    promotionQueenCandidate = NULL;
    matchingCount = 0;
    allMatchesArePromotions = 1;
    for (index = 0; index < getMoveCount(&candidates); ++index) {
        Move *candidate = getMove(&candidates, index);

        if (candidate == NULL
            || !positionEqual(candidate->from, command.from)
            || !positionEqual(candidate->to, command.to)
            || candidate->movedPiece.type != movingPiece.type
            || candidate->movedPiece.color != movingPiece.color) {
            continue;
        }

        ++matchingCount;
        if (candidate->specialType == PROMOTION_QUEEN) {
            promotionQueenCandidate = candidate;
        } else if (!is_promotion_move(candidate->specialType)) {
            allMatchesArePromotions = 0;
        }

        if (matchingCount == 1) {
            *resolvedMove = *candidate;
        }
    }

    if (matchingCount == 1) {
        return 0;
    }

    if (matchingCount > 1 && allMatchesArePromotions && promotionQueenCandidate != NULL) {
        *resolvedMove = *promotionQueenCandidate;
        return 0;
    }

    return 1;
}

/* Print one concise CLI move summary for AI moves and hint suggestions. */
static void print_move_summary(const char *prefix, Move move) {
    char fromText[8];
    char toText[8];
    const char *specialText = special_move_label(move.specialType);

    format_position_text(move.from, fromText);
    format_position_text(move.to, toText);

    printf("%s[%s]%s %s %s %s -> %s",
        ANSI_ACCENT,
        prefix,
        ANSI_RESET,
        color_label(move.movedPiece.color),
        piece_label(move.movedPiece.type),
        fromText,
        toText);

    if (move.captureCount > 0) {
        printf(" | captures: %d", move.captureCount);
    }

    if (specialText != NULL) {
        printf(" | special: %s", specialText);
    }

    printf("\n");
}

/* Collect the next main-menu event for the CLI frontend. */
static int collect_main_menu_event(Controller *controller) {
    int selection;

    if (cliGetMainMenuSelection(&selection) != 0) {
        return 1;
    }

    if (selection == 1) {
        return controllerRequestNewGame(controller);
    }

    return controllerRequestExit(controller);
}

/* Collect the next game-mode event for the CLI frontend. */
static int collect_mode_selection_event(Controller *controller, GameConfig *pendingConfig) {
    int selection;

    if (pendingConfig == NULL || cliGetGameModeSelection(&selection) != 0) {
        return 1;
    }

    switch (selection) {
        case 1:
            initGameConfigForMode(pendingConfig, MODE_HUMAN_VS_HUMAN);
            return controllerRequestNewGame(controller);
        case 2:
            initGameConfigForMode(pendingConfig, MODE_HUMAN_VS_COMPUTER);
            return controllerRequestNewGame(controller);
        case 3:
            initGameConfigForMode(pendingConfig, MODE_COMPUTER_VS_COMPUTER);
            return controllerRequestNewGame(controller);
        case 4:
            return controllerRequestBack(controller);
        case 5:
            return controllerRequestExit(controller);
        default:
            return 1;
    }
}

/* Re-check the active gameplay timer after a blocking CLI prompt. */
static int enqueue_timer_expiry_if_needed(Controller *controller) {
    GameState *state;

    if (controller == NULL) {
        return 1;
    }

    state = &controller->state;
    if (state->systemState != GAMEPLAY_STATE || !state->config.timerEnabled) {
        return 0;
    }

    if (updateTurnTimer(state) != 0) {
        return 1;
    }

    if (isTimeUp(state) == 1) {
        return controllerEnqueueEvent(controller, createSystemEvent(EVENT_TIMER_EXPIRED));
    }

    return 0;
}

/* Collect setup fields, rebuild the controller, and continue into gameplay. */
static int collect_setup_event(Controller *controller, GameConfig *pendingConfig) {
    GameConfig config;

    if (pendingConfig == NULL) {
        return 1;
    }

    config = *pendingConfig;
    if (cliGetGameSetupConfig(&config) != 0) {
        return 1;
    }

    *pendingConfig = config;
    return controllerStartConfiguredGame(controller, pendingConfig);
}

/* Collect one gameplay action and map it to the next CLI event if any. */
static int collect_gameplay_event(Controller *controller) {
    const GameState *state;
    int action;
    Command command;
    Move resolvedMove;
    Move hintMove;

    if (controller == NULL) {
        return 1;
    }

    state = controllerGetState(controller);
    if (render_gameplay_view(state) != 0) {
        return 1;
    }

    if (current_turn_is_ai(state)) {
        return 0;
    }

    if (cliGetGameplayAction(&action) != 0) {
        return 1;
    }

    state = controllerGetState(controller);
    if (state == NULL) {
        return 1;
    }

    switch (action) {
        case 1:
            if (cliShowMoveFormatHint() != 0) {
                return 1;
            }
            for (;;) {
                if (cliGetMoveCommand(&command) != 0) {
                    cliShowErrorMessage(ERR_INVALID_MOVE_FORMAT);
                    continue;
                }

                if (resolve_cli_move_command(state, command, &resolvedMove) != 0) {
                    cliShowErrorMessage(classify_move_error(state, command));
                    continue;
                }

                /* Keep move input as a low-level event so the outer CLI tick can
                 * print the exact processed move, timeout, or error event. */
                return controllerEnqueueEvent(controller, createMoveInputEvent(command));
            }
        case 2:
            if (enqueue_timer_expiry_if_needed(controller) != 0) {
                return 1;
            }
            if (!controller_queue_is_empty(controller)) {
                return 0;
            }
            if (controllerRequestUndo(controller) != 0) {
                cliShowErrorMessage(ERR_UNDO_UNAVAILABLE);
            }
            return 0;
        case 3:
            if (enqueue_timer_expiry_if_needed(controller) != 0) {
                return 1;
            }
            if (!controller_queue_is_empty(controller)) {
                return 0;
            }
            return controllerRequestLeaveGame(controller);
        case 4:
            if (enqueue_timer_expiry_if_needed(controller) != 0) {
                return 1;
            }
            if (!controller_queue_is_empty(controller)) {
                return 0;
            }
            return controllerRequestExit(controller);
        case 5:
            if (enqueue_timer_expiry_if_needed(controller) != 0) {
                return 1;
            }
            if (!controller_queue_is_empty(controller)) {
                return 0;
            }
            state = controllerGetState(controller);
            if (state == NULL) {
                return 1;
            }
            if (controllerGetHint(controller, &hintMove) != 0) {
                cliShowErrorMessage(ERR_HINT_UNAVAILABLE);
                return 0;
            }
            print_move_summary("Hint", hintMove);
            return 0;
        default:
            return 1;
    }
}

/* Collect the next end-game menu event for the CLI frontend. */
static int collect_endgame_event(const GameState *state, Controller *controller) {
    int selection;

    if (cliShowEndGameMenu(state, &selection) != 0) {
        return 1;
    }

    switch (selection) {
        case 1:
            return controllerRequestNewGame(controller);
        case 2:
            return controllerRequestBack(controller);
        case 3:
            return controllerRequestExit(controller);
        default:
            return 1;
    }
}

/* Classify one rejected move command into the most precise current public error code. */
static ErrorCode classify_move_error(const GameState *state, Command command) {
    SelectionResult selectionResult;

    if (state == NULL) {
        return ERR_FATAL;
    }

    if (command.type != CMD_MOVE) {
        return ERR_INVALID_MOVE_FORMAT;
    }

    if (!isValidPosition(command.from) || !isValidPosition(command.to)) {
        return ERR_POSITION_OUT_OF_BOUNDS;
    }

    selectionResult = validateSelection(state, command.from);
    switch (selectionResult) {
        case SELECT_EMPTY:
            return ERR_EMPTY_SELECTION;
        case SELECT_OPPONENT_PIECE:
            return ERR_OPPONENT_PIECE;
        case SELECT_OUT_OF_BOUNDS:
            return ERR_POSITION_OUT_OF_BOUNDS;
        case SELECT_VALID:
        default:
            return ERR_ILLEGAL_MOVE;
    }
}

/* Collect the next external CLI event for the active controller state. */
static int collect_next_cli_event(Controller *controller, GameConfig *pendingConfig) {
    const GameState *state;

    if (controller == NULL || pendingConfig == NULL) {
        return 1;
    }

    if (!controller_queue_is_empty(controller)) {
        return 0;
    }

    state = controllerGetState(controller);
    switch (state->systemState) {
        case MAIN_MENU_STATE:
            return collect_main_menu_event(controller);
        case GAME_MODE_SELECTION_STATE:
            return collect_mode_selection_event(controller, pendingConfig);
        case GAME_SETUP_STATE:
            return collect_setup_event(controller, pendingConfig);
        case GAMEPLAY_STATE:
            return collect_gameplay_event(controller);
        case END_GAME_MENU_STATE:
            return collect_endgame_event(state, controller);
        case INIT_STATE:
        case GAME_TERMINATION_STATE:
        case EXIT_STATE:
            return 0;
        default:
            return 1;
    }
}

/* Translate common FSM failures into user-visible CLI feedback. */
static void report_processing_error(const GameState *state, Event event) {
    if (event.type == EVENT_MOVE_INPUT) {
        cliShowErrorMessage(classify_move_error(state, event.data.command));
        return;
    }

    if (event.type == EVENT_UNDO) {
        cliShowErrorMessage(ERR_UNDO_UNAVAILABLE);
        return;
    }

    if (event.type == EVENT_HINT) {
        cliShowErrorMessage(ERR_HINT_UNAVAILABLE);
        return;
    }

    if (state != NULL && state->config.timerEnabled && state->systemState == GAMEPLAY_STATE && isTimeUp(state) == 1) {
        cliShowErrorMessage(ERR_TIME_UP);
        return;
    }

    cliShowErrorMessage(ERR_FATAL);
}

/* Run the standalone CLI application loop until the FSM reaches EXIT_STATE. */
int runCliApp(void) {
    Controller controller;
    GameConfig config;
    GameConfig pendingConfig;

    initDefaultGameConfig(&config);
    pendingConfig = config;
    initController(&controller, &config);

    while (controllerGetState(&controller)->systemState != EXIT_STATE) {
        Event event;
        Color timerExpiredSide;
        const GameState *state;

        state = controllerGetState(&controller);
        timerExpiredSide = state->currentTurn;
        if (controllerTick(&controller, &event) != 0) {
            report_processing_error(controllerGetState(&controller), event);
            continue;
        }

        if (event.type == EVENT_TIMER_EXPIRED) {
            print_timeout_skip_message(timerExpiredSide);
            continue;
        }

        if (event.type == EVENT_AI_MOVE) {
            printf("%s[AI]%s %s is thinking...\n",
                ANSI_ACCENT,
                ANSI_RESET,
                color_label(event.data.move.movedPiece.color));
            print_move_summary("AI Move", event.data.move);
            continue;
        }

        if (collect_next_cli_event(&controller, &pendingConfig) != 0) {
            cliShowErrorMessage(ERR_FATAL);
            return 1;
        }
    }

    return 0;
}
