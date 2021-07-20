#include "ui.h"

#include <ncurses.h>

namespace sv {

UI::UI() {
  // Init ncurses
  initscr();
  raw();
  keypad(stdscr, true);
  noecho();
  curs_set(0); // Hide cursor

  // Default splits on startup.
  getmaxyx(stdscr, term_h_, term_w_);
  src_pos_x_ = term_w_ * 30 / 100;
  wave_pos_y_ = term_h_ * 50 / 100;

  // Initial render
  DrawPanes();
}

UI::~UI() {
  // Cleanup ncurses
  endwin();
}

void UI::EventLoop() {
  int ch = 0;
  while (1) {
    ch = getch();
    if (ch == KEY_RESIZE) {
      float x = (float)src_pos_x_ / term_w_;
      float y = (float)wave_pos_y_ / term_h_;
      getmaxyx(stdscr, term_h_, term_w_);
      src_pos_x_ = (int)(term_w_ * x);
      wave_pos_y_ = (int)(term_h_ * y);
      DrawPanes();

      continue;
    }
    switch (ch) {
      case 0x221: // ctrl-left
        if (src_pos_x_ > 5) {
          src_pos_x_--;
          DrawPanes();
        }
        break;
      case 0x230: // ctrl-right
        if (src_pos_x_ < term_w_ - 5) {
          src_pos_x_++;
          DrawPanes();
        }
        break;
      case 0x236: // ctrl-up
        if (wave_pos_y_ > 5) {
          wave_pos_y_--;
          DrawPanes();
        }
        break;
      case 0x20d: // ctrl-down
        if (wave_pos_y_ < term_h_ - 5) {
          wave_pos_y_++;
          DrawPanes();
        }
        break;
    }
    // For now:
    if (ch == 'q') break;
  }
}

void UI::DrawPanes() {
  clear();
  for (int row = 0; row < wave_pos_y_; ++row) {
    mvaddch(row, src_pos_x_, ACS_VLINE);
  }
  move(wave_pos_y_, 0);
  for (int col = 0; col < term_w_; ++col) {
    addch(col == src_pos_x_ ? ACS_BTEE : ACS_HLINE);
  }
  refresh();
}

} // namespace sv
