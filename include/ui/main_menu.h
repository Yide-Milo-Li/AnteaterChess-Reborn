
#ifndef CHESS_UI_MAIN_MENU_H
#define CHESS_UI_MAIN_MENU_H
// Sets the main menu selection index (0=New Game, 1=Quit)
void setMainMenuSelection(int index);
// Gets the last main menu selection index (0=New Game, 1=Quit), returns 0 on success, -1 on error
int getMainMenuSelection(int *selection);
void resetMainMenuSelection();

#endif
