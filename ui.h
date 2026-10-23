#ifndef _UI_H_
#define _UI_H_

#include <ncurses.h>

extern WINDOW *input_win;

void ui_init();
void ui_cleanup();
void ui_print(const char *msg);
void ui_set_status(const char *status);

#endif
