#include "ui/main_menu.h"

static int main_menu_selection = -1; // -1 means no selection yet

void setMainMenuSelection(int index) {
	main_menu_selection = index;
}

int getMainMenuSelection(int *selection){
	if (!selection) return -1;
	*selection = main_menu_selection;
	return 0;
}
