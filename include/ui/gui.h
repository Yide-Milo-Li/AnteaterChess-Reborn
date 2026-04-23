
#ifndef CHESS_UI_GUI_H
#define CHESS_UI_GUI_H

typedef struct Gui Gui;

Gui *gui_create(int *argc, char ***argv);
void gui_destroy(Gui *gui);
void gui_run(Gui *gui);

#endif
