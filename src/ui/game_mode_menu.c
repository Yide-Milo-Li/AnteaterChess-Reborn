#include "ui/game_mode_menu.h"
#include "ui/dialog.h"
#include "core/gameconfig.h"

static GameMode game_mode_selection = -1; // -1 means no selection yet

// Sets the game mode selection index (0=Human vs. Computer, 1=Human vs. Human, 2=Computer vs. Computer), stores the enum directly
void setGameModeSelection(int index) {
	if (index >= 0 && index <= 2) {
		game_mode_selection = (GameMode)index;
	}
}

// Gets the last game mode selection, returns 0 on success, -1 on error, stores GameMode enum value
int getGameModeSelection(int *selection) {
	if (!selection) return -1;
	*selection = game_mode_selection;
	return 0;
}

void resetGameModeSelection() {
	game_mode_selection = -1;
}
