#include "../../include/ui/dialog.h"

int quit_choice = 0;
int leave_choice = 0;

void setQuitChoice(int choice) {
    quit_choice = choice;
}

void setLeaveChoice(int choice) {
    leave_choice = choice;
}

int confirmLeaveGame(int *result) {
    if (!result) {
        return -1;
    }
    *result = leave_choice;
    return 0;
}

int confirmQuitGame(int *result) {
    if (!result) {
        return -1;
    }
    *result = quit_choice;
    return 0;
}