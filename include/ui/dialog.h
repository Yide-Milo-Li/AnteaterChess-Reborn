#ifndef CHESS_UI_DIALOG_H
#define CHESS_UI_DIALOG_H

extern int quit_choice;
extern int leave_choice;
extern int back_button_clicked;

void setQuitChoice(int choice);
void setLeaveChoice(int choice);
void setBackButtonClicked(int clicked);

int confirmLeaveGame(int *result);
int confirmQuitGame(int *result);
int isBackButtonClicked();
void resetBackButtonClicked();

#endif
