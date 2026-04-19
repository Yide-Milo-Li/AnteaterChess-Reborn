#ifndef CHESS_UI_DIALOG_H
#define CHESS_UI_DIALOG_H

extern int quit_choice;
extern int leave_choice;

void setQuitChoice(int choice);
void setLeaveChoice(int choice);

int confirmLeaveGame(int *result);
int confirmQuitGame(int *result);

#endif
