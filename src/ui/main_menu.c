#include "ui/main_menu.h"

static int main_menu_selection = -1; // -1 means no selection yet

// Sets the main menu selection index (0=New Game, 1=Quit), shouldn't be called outside of GUI callbacks
void setMainMenuSelection(int index) {
	main_menu_selection = index;
}

// Gets the last main menu selection index (0=New Game, 1=Quit), returns 0 on success, -1 on error
int getMainMenuSelection(int *selection){
	if (!selection) return -1;
	*selection = main_menu_selection;
	return 0;
}

void resetMainMenuSelection() {
	main_menu_selection = -1;
}
