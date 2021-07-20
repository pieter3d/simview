#include "ui.h"

#include <ncurses.h>

namespace sv {

UI::UI() {
  int term_w, term_h;
  getmaxyx(stdscr, term_h, term_w);
  // Default splits on startup.
  src_pos_x_ = term_w * 30 / 100;
  wave_pos_y_ = term_h * 50 / 100;

  // Init ncurses
  initscr();
  raw();
  keypad(stdscr, true);
  noecho();
}

UI::~UI() {
  // Cleanup ncurses
  endwin();
}

void UI::EventLoop() {
  // TODO: just get a character and return for now.
  getch();
}

} // namespace sv
