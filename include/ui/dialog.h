#ifndef CHESS_UI_DIALOG_H
#define CHESS_UI_DIALOG_H

void setQuitChoice(int choice);
void setLeaveChoice(int choice);
void setBackButtonClicked(int clicked);

int confirmLeaveGame(int *result);
int confirmQuitGame(int *result);
int isBackButtonClicked();
void resetBackButtonClicked();
void resetLeaveChoice();

#endif
