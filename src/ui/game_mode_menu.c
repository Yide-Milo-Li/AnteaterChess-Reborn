#include "ui/game_mode_menu.h"
#include "core/gameconfig.h"

static int game_mode_selection = -1; // -1 means no selection yet

// Sets the game mode selection index (0=Human vs. Computer, 1=Human vs. Human, 2=Computer vs. Computer, 3=Back), shouldn't be called outside of GUI callbacks
void setGameModeSelection(int index) {
	game_mode_selection = index;
}

// Gets the last game mode selection, returns 0 on success, -1 on error, result is set to the GameMode enum value or 3 for Back in the provided pointer
int getGameModeSelection(int *selection) {
	if (!selection) return -1;
	if (game_mode_selection == 0) {
		*selection = MODE_HUMAN_VS_COMPUTER;
	} else if (game_mode_selection == 1) {
		*selection = MODE_HUMAN_VS_HUMAN;
	} else if (game_mode_selection == 2) {
		*selection = MODE_COMPUTER_VS_COMPUTER;
	} else {
		*selection = game_mode_selection; // For Back (3) or others
	}
	return 0;
}
