#include "ui/game_mode_menu.h"

static int game_mode_selection = -1; // -1 means no selection yet

void setGameModeSelection(int index) {
	game_mode_selection = index;
}

int getGameModeSelection(int *selection) {
	if (!selection) return -1;
	*selection = game_mode_selection;
	return 0;
}
