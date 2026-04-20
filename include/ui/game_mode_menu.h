#ifndef CHESS_UI_GAME_MODE_MENU_H
#define CHESS_UI_GAME_MODE_MENU_H


// Sets the game mode selection index (0=Human vs. Computer, 1=Human vs. Human, 2=Computer vs. Computer, 3=Back)
void setGameModeSelection(int index);
// Gets the last game mode selection index, returns 0 on success, -1 on error
int getGameModeSelection(int *selection);

#endif
