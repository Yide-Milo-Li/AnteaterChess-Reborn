// Forward declaration for main menu rendering
void setup_main_menu(struct Gui *gui);

#include "ui/board_renderer.h"
#include "core/gamestate.h"
#include "system/system_state.h"
#include "ui/gui.h"

// Static GUI pointer for the application window
static Gui *gui = NULL;

int renderBoard(const GameState *state){
	if (!state) return -1;
	int render_success = 0;
	switch (state->systemState) {
		case INIT_STATE:
			// Initialize the window if not already done, but do not add widgets yet
			if (!gui) {
				int argc = 0;
				char **argv = NULL;
				gui = gui_create(&argc, &argv);
				if (!gui) return -1;
			}
			break;
		case MAIN_MENU_STATE:
			// Render the main menu
			if (gui) {
				setup_main_menu(gui);
			} else {
				render_success = -1;
			}
			break;
		case GAME_MODE_SELECTION_STATE:
			// Handle game mode selection rendering, should display options for different game modes (e.g., Player vs Player, Player vs AI, AI vs AI)
			break;
		case GAME_SETUP_STATE:
			// Handle game setup rendering, should display options for configuring the game (e.g., choosing player colors, setting AI difficulty, enabling timers)
			break;
		case GAMEPLAY_STATE:
			// Handle gameplay rendering, should display the chess board, pieces, and any relevant game information (e.g., current turn, move history, timers), some buttons including undo, leave game, and hint will also be included in gameplay rendering
			break;
        case GAME_TERMINATION_STATE:
			// Handle game termination rendering, this happens after GAMEPLAY_STATE when a game ends but before END_GAME_MENU_STATE, clear board, pieces, and other gui elements, and prepare for end game menu rendering
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
