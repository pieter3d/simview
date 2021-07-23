#include "ui.h"

#include <memory>
#include <ncurses.h>

namespace sv {
namespace {

constexpr int kBorderPair = 1;

void setup_colors() {
  if (has_colors()) {
    start_color();
    init_pair(kBorderPair, COLOR_MAGENTA, COLOR_BLACK);
  }
}
} // namespace

UI::UI() {
  // Init ncurses
  initscr();
  cbreak();
  keypad(stdscr, true);
  noecho();
  curs_set(0); // Hide cursor
  setup_colors();

  // Default splits on startup.
  getmaxyx(stdscr, term_h_, term_w_);
  src_pos_x_ = term_w_ * 30 / 100;
  wave_pos_y_ = term_h_ * 50 / 100;

  // Create windows and associated panels
  hierarchy_ = std::make_unique<Hierarchy>(
      newwin(wave_pos_y_ - 1, src_pos_x_ - 1, 0, 0));
  source_ = std::make_unique<Source>(
      newwin(wave_pos_y_ - 1, term_w_ - src_pos_x_, 0, src_pos_x_ + 1));
  waves_ = std::make_unique<Waves>(
      newwin(term_h_ - wave_pos_y_ - 1, term_w_, wave_pos_y_ + 1, 0));
  focused_panel_ = hierarchy_.get();

  // Initial render
  DrawPanes(false);
}

UI::~UI() {
  // Cleanup ncurses
  endwin();
}

void UI::EventLoop() {
  int ch = 0;
  while (1) {
    bool resize = false;
    bool redraw = false;
    ch = getch();
    if (ch == KEY_RESIZE) {
      float x = (float)src_pos_x_ / term_w_;
      float y = (float)wave_pos_y_ / term_h_;
      getmaxyx(stdscr, term_h_, term_w_);
      src_pos_x_ = (int)(term_w_ * x);
      wave_pos_y_ = (int)(term_h_ * y);
      resize = true;
      continue;
    }
    switch (ch) {
    case 0x221: // ctrl-left
      if (src_pos_x_ > 5) {
        src_pos_x_--;
        resize = true;
      }
      break;
    case 0x230: // ctrl-right
      if (src_pos_x_ < term_w_ - 5) {
        src_pos_x_++;
        resize = true;
      }
      break;
    case 0x236: // ctrl-up
      if (wave_pos_y_ > 5) {
        wave_pos_y_--;
        resize = true;
      }
      break;
    case 0x20d: // ctrl-down
      if (wave_pos_y_ < term_h_ - 5) {
        wave_pos_y_++;
        resize = true;
      }
      break;
    }
    // For now:
    if (ch == 'q') break;
    if (redraw || resize) {
      DrawPanes(resize);
    }
  }
}

void UI::DrawPanes(bool resize) {
  if (resize) {
    wresize(hierarchy_->Window(), wave_pos_y_ - 1, src_pos_x_ - 1);
    wresize(source_->Window(), wave_pos_y_ - 1, term_w_ - src_pos_x_ - 1);
    mvwin(source_->Window(), 0, src_pos_x_ + 1);
    wresize(waves_->Window(), term_h_ - wave_pos_y_ - 1, term_w_);
    mvwin(waves_->Window(), wave_pos_y_ + 1, 0);
  }
  erase();
  mvvline(0, src_pos_x_, ACS_VLINE, wave_pos_y_);
  mvhline(wave_pos_y_, 0, ACS_HLINE, src_pos_x_);
  mvaddch(wave_pos_y_, src_pos_x_, ACS_BTEE);
  hline(ACS_HLINE, term_w_ - src_pos_x_ - 1);
  wnoutrefresh(stdscr);
  hierarchy_->Draw();
  source_->Draw();
  waves_->Draw();
  wnoutrefresh(hierarchy_->Window());
  wnoutrefresh(source_->Window());
  wnoutrefresh(waves_->Window());
  doupdate();
}

void UI::SetDesign(SURELOG::Design *d) { hierarchy_->SetDesign(d); }

} // namespace sv
