#include "../../include/ui/dialog.h"

static int quit_choice = 0;
static int leave_choice = 0;
static int back_button_clicked = 0;
// Sets the quit choice (1 for yes, 0 for no), shouldn't be called outside of GUI callbacks
void setQuitChoice(int choice) {
    quit_choice = choice;
}

//sets the leave choice (1 for yes, 0 for no), shouldn't be called outside of GUI callbacks
void setLeaveChoice(int choice) {
    leave_choice = choice;
}

void setBackButtonClicked(int clicked) {
    back_button_clicked = clicked;
}

// Gets the user's choice for confirming leaving the game, returns 0 on success, -1 on error, result is set to 1 for yes and 0 for no sotred in the provided pointer
int confirmLeaveGame(int *result) {
    if (!result) {
        return -1;
    }
    *result = leave_choice;
    return 0;
}

// Gets the user's choice for confirming quitting the game, returns 0 on success, -1 on error, result is set to 1 for yes and 0 for no stored in the provided pointer
int confirmQuitGame(int *result) {
    if (!result) {
        return -1;
    }
    *result = quit_choice;
    return 0;
}

int isBackButtonClicked() {
    return back_button_clicked;
}

void resetBackButtonClicked() {
    back_button_clicked = 0;
}

void resetLeaveChoice() {
    leave_choice = 0;
}
