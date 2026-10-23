#include <ncurses.h>
#include <string.h>
#include "ui.h"

WINDOW *msg_win;
WINDOW *input_win;
static WINDOW *status_win;
static int msg_rows;

void ui_init() {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  start_color();

  init_pair(1, COLOR_CYAN, COLOR_BLACK);
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_GREEN, COLOR_BLACK);

  int rows, cols;
  getmaxyx(stdscr, rows, cols);
  msg_rows = rows - 4;

  msg_win = newwin(msg_rows, cols, 0, 0);
  scrollok(msg_win, TRUE);
  wattron(msg_win, COLOR_PAIR(2));

  WINDOW *sep1 = newwin(1, cols, msg_rows, 0);
  whline(sep1, ACS_HLINE, cols);
  wrefresh(sep1);

  input_win = newwin(1, cols, msg_rows + 1, 0);
  wattron(input_win, COLOR_PAIR(3));

  WINDOW *sep2 = newwin(1, cols, msg_rows + 2, 0);
  whline(sep2, ACS_HLINE, cols);
  wrefresh(sep2);

  status_win = newwin(1, cols, rows - 1, 0);
  wbkgd(status_win, COLOR_PAIR(1));
  wattron(status_win, COLOR_PAIR(1));

  mousemask(0, NULL);

  printf("\033[?1003l"); // disable mouse tracking
  printf("\033[?1000l"); // disable mouse click reporting
  printf("\033[?1015l"); // disable urxvt scroll
  printf("\033[?1006l"); // disable SGR mouse
  fflush(stdout);

  ui_set_status("Connecting...");
  refresh();
}

void ui_cleanup() {
  endwin();
}

void ui_print(const char *msg) {
  const char *p = msg;
  while (*p) {
    const char *nl = strchr(p, '\n');
    if (nl) {
      waddnstr(msg_win, p, nl - p);
      waddch(msg_win, '\n');
      p = nl + 1;
    } else {
      waddstr(msg_win, p);
      waddch(msg_win, '\n');
      break;
    }
  }
  waddch(msg_win, '\n'); // blank line after each message
  wrefresh(msg_win);

  werase(input_win);
  wattron(input_win, COLOR_PAIR(3));
  wprintw(input_win, "> ");
  wrefresh(input_win);
}

void ui_set_status(const char *status) {
  werase(status_win);
  wattron(status_win, COLOR_PAIR(1));
  wbkgd(status_win, COLOR_PAIR(1));
  wprintw(status_win, " %s", status);
  wrefresh(status_win);
}
