
#include "ui/board_renderer.h"
#include "core/gamestate.h"
#include "system/system_state.h"
#include "ui/game_setup_menu.h"
#include "ui/gui.h"

// Static GUI pointer for the application window
static Gui *gui = NULL;
// Getter for the static gui pointer
Gui *get_board_renderer_gui(void) {
	return gui;
}

// Event processing and validation functions
void br_process_events(void) {
    gui_process_events();
}

// Wrapper for GUI window validation, returns 1 if the window is valid and 0 otherwise
int br_window_is_valid(Gui *gui) {
    return gui_window_is_valid(gui);
}

// Main rendering function for the board and UI, takes in the current GameState to determine what to render based on the systemState field. Returns 0 on success and -1 on failure (e.g., invalid state or rendering error)
int renderBoard(const GameState *state){
	if (!state) return -1;
	int render_success = 0;
	switch (state->systemState) {
		case INIT_STATE:
			g_print("Window initialized (INIT_STATE).\n");
			// Initialize the window if not already done, but do not add widgets yet.
			// This state will also be used to load resources (e.g., images for pieces), to be implemented in the future.
			if (!gui) {
				int argc = 0;
				char **argv = NULL;
				gui = gui_create(&argc, &argv);
				if (!gui) return -1;
			}
			// TODO: Load resources such as images for pieces here.
			break;
		case MAIN_MENU_STATE:
			// Render the main menu
			if (gui && gui->window) {
				setup_main_menu(gui);
				g_print("Main menu rendered (MAIN_MENU_STATE).\n");
				gui_reset_selections();
			} else {
				render_success = -1;
			}
			break;
		case GAME_MODE_SELECTION_STATE:
			// Render the game mode selection menu
			if (gui && gui->window) {
				setup_game_mode_menu(gui);
				g_print("Game mode selection rendered (GAME_MODE_SELECTION_STATE).\n");
				gui_reset_selections();
			} else {
				render_success = -1;
			}
			break;
		case GAME_SETUP_STATE:
                        initGameSetupConfig(state->config.mode);
			// Handle game setup rendering, should display options for configuring the game (e.g., choosing player colors, setting AI difficulty, enabling timers)
			if (gui && gui->window) {
				switch (state->config.mode) {
					case MODE_HUMAN_VS_HUMAN:
						// Render setup for Human vs. Human
						setup_game_setup_menu_human_vs_human(gui);
						g_print("Game setup rendered for Human vs. Human (GAME_SETUP_STATE).\n");
						break;
					case MODE_HUMAN_VS_COMPUTER:
						// Render setup for Human vs. Computer
						setup_game_setup_menu_human_vs_computer(gui);
						g_print("Game setup rendered for Human vs. Computer (GAME_SETUP_STATE).\n");
						break;
					case MODE_COMPUTER_VS_COMPUTER:
						// Render setup for Computer vs. Computer
						setup_game_setup_menu_computer_vs_computer(gui);
						g_print("Game setup rendered for Computer vs. Computer (GAME_SETUP_STATE).\n");
						break;
					default:
						g_print("Unknown game mode in setup (GAME_SETUP_STATE).\n");
						render_success = -1;
						break;
				}
				gui_reset_selections();
			} else {
				render_success = -1;
			}
			break;
		case GAMEPLAY_STATE:
			// Handle gameplay rendering, should display the chess board, pieces, and any relevant game information (e.g., current turn, move history, timers), some buttons including undo, leave game, and hint will also be included in gameplay rendering
			if (gui && gui->window) {
				setup_gameplay_ui(gui, state);
				g_print("Gameplay UI rendered (GAMEPLAY_STATE).\n");
			} else {
				render_success = -1;
			}
			break;
        case GAME_TERMINATION_STATE:
			// Handle game termination rendering, this happens after GAMEPLAY_STATE when a game ends but before END_GAME_MENU_STATE, user is no longer able to interact with the gui elements, and prepare for end game menu rendering
            break;
		case END_GAME_MENU_STATE:
			// Handle end game menu rendering, should display the result of the game, and options to start a new game or return to the main menu
			break;
		case EXIT_STATE:
			// Handle exit state rendering, destroy the main window and clean up resources, this will be triggered when the user confirms quitting the game from the main menu
			break;
		default:
			// Unknown state - handle error
            return -1;
			break;
	}
	return render_success;
}
