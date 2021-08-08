#include "ui.h"

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "color.h"
#include <memory>
#include <ncurses.h>

namespace sv {

UI::UI() {
  setlocale(LC_ALL, "");
  // Init ncurses
  initscr();
  cbreak();
  keypad(stdscr, true);
  noecho();
  nonl();      // don't translate the enter key
  curs_set(0); // Hide cursor
  SetupColors();

  // Default splits on startup.
  getmaxyx(stdscr, term_h_, term_w_);
  src_pos_x_ = term_w_ * 30 / 100;
  wave_pos_y_ = term_h_ * 50 / 100;

  // Create windows and associated panels
  hierarchy_ =
      std::make_unique<Hierarchy>(newwin(wave_pos_y_, src_pos_x_ - 1, 0, 0));
  source_ = std::make_unique<Source>(
      newwin(wave_pos_y_, term_w_ - src_pos_x_, 0, src_pos_x_ + 1));
  waves_ = std::make_unique<Waves>(
      newwin(term_h_ - wave_pos_y_ - 2, term_w_, wave_pos_y_ + 1, 0));
  focused_panel_ = hierarchy_.get();
  prev_focused_panel_ = focused_panel_;
  focused_panel_->SetFocus(true);

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

    // TODO: remove
    auto now = absl::Now();
    if (now - last_ch > absl::Milliseconds(50)) {
      tmp_ch.clear();
    }
    tmp_ch.push_back(ch);
    last_ch = now;

    switch (ch) {
    case KEY_RESIZE: {
      float x = (float)src_pos_x_ / term_w_;
      float y = (float)wave_pos_y_ / term_h_;
      getmaxyx(stdscr, term_h_, term_w_);
      src_pos_x_ = (int)(term_w_ * x);
      wave_pos_y_ = (int)(term_h_ * y);
      resize = true;
      break;
    }
    case 0x8:   // ctrl-H
    case 0x221: // ctrl-left
      if (src_pos_x_ > 5) {
        src_pos_x_--;
        resize = true;
      }
      break;
    case 0xc:   // ctrl-L
    case 0x230: // ctrl-right
      if (src_pos_x_ < term_w_ - 5) {
        src_pos_x_++;
        resize = true;
      }
      break;
    case 0xb:   // ctrl-K
    case 0x236: // ctrl-up
      if (wave_pos_y_ > 5) {
        wave_pos_y_--;
        resize = true;
      }
      break;
    case 0xa:   // ctrl-J
    case 0x20d: // ctrl-down
      if (wave_pos_y_ < term_h_ - 5) {
        wave_pos_y_++;
        resize = true;
      }
      break;
    case 'H':
    case 0x189: // shift-left
      if (focused_panel_ == source_.get()) {
        prev_focused_panel_ = focused_panel_;
        focused_panel_ = hierarchy_.get();
        redraw = true;
      }
      break;
    case 'L':
    case 0x192: // shift-right
      if (focused_panel_ == hierarchy_.get()) {
        prev_focused_panel_ = focused_panel_;
        focused_panel_ = source_.get();
        redraw = true;
      }
      break;
    case 'K':
    case 0x151: // shift-up
      if (focused_panel_ == waves_.get()) {
        focused_panel_ = prev_focused_panel_;
        prev_focused_panel_ = waves_.get();
        redraw = true;
      }
      break;
    case 'J':
    case 0x150: // shift-down
      if (focused_panel_ != waves_.get()) {
        prev_focused_panel_ = focused_panel_;
        focused_panel_ = waves_.get();
        redraw = true;
      }
      break;
    case 0x9: // tab
      prev_focused_panel_ = focused_panel_;
      if (focused_panel_ == hierarchy_.get()) {
        focused_panel_ = source_.get();
      } else if (focused_panel_ == source_.get()) {
        focused_panel_ = waves_.get();
      } else if (focused_panel_ == waves_.get()) {
        focused_panel_ = hierarchy_.get();
      }
      redraw = true;
      break;
    case 0x161: // shift-tab
      prev_focused_panel_ = focused_panel_;
      if (focused_panel_ == hierarchy_.get()) {
        focused_panel_ = waves_.get();
      } else if (focused_panel_ == source_.get()) {
        focused_panel_ = hierarchy_.get();
      } else if (focused_panel_ == waves_.get()) {
        focused_panel_ = source_.get();
      }
      redraw = true;
      break;
    default:
      focused_panel_->UIChar(ch);
      redraw = true;
      break;
    }
    prev_focused_panel_->SetFocus(false);
    focused_panel_->SetFocus(true);
    if (focused_panel_->TransferPending()) {
      if (focused_panel_ == hierarchy_.get()) {
        auto item = hierarchy_->ItemForSource();
        source_->SetItem(item.first, item.second);
      }
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
    wresize(hierarchy_->Window(), wave_pos_y_, src_pos_x_);
    wresize(source_->Window(), wave_pos_y_, term_w_ - src_pos_x_ - 1);
    mvwin(source_->Window(), 0, src_pos_x_ + 1);
    wresize(waves_->Window(), term_h_ - wave_pos_y_ - 2, term_w_);
    mvwin(waves_->Window(), wave_pos_y_ + 1, 0);
    hierarchy_->Resized();
    source_->Resized();
    waves_->Resized();
  }
  erase();
  // Render the dividing lines
  if (focused_panel_ != waves_.get()) {
    color_set(kFocusBorderPair, nullptr);
  } else {
    color_set(kBorderPair, nullptr);
  }
  mvvline(0, src_pos_x_, ACS_VLINE, wave_pos_y_);
  if (focused_panel_ != source_.get()) {
    color_set(kFocusBorderPair, nullptr);
  } else {
    color_set(kBorderPair, nullptr);
  }
  mvhline(wave_pos_y_, 0, ACS_HLINE, src_pos_x_);
  color_set(kFocusBorderPair, nullptr);
  mvaddch(wave_pos_y_, src_pos_x_, ACS_BTEE);
  if (focused_panel_ != hierarchy_.get()) {
    color_set(kFocusBorderPair, nullptr);
  } else {
    color_set(kBorderPair, nullptr);
  }
  hline(ACS_HLINE, term_w_ - src_pos_x_ - 1);

  // Display most recent key codes for development ease. TODO: remove
  std::string s;
  for (int code : tmp_ch)
    s.append(absl::StrFormat("0x%x ", code));
  mvprintw(wave_pos_y_, 0, "codes: %s", s.c_str());

  // Render the tooltip.
  auto tooltip = "/:search  " + focused_panel_->Tooltip();
  for (int x = 0; x < term_w_; ++x) {
    // Look for a key (indicated by colon following).
    const bool is_key = x < tooltip.size() - 1 ? tooltip[x + 1] == ':' : false;
    color_set(is_key ? kTooltipKeyPair : kTooltipPair, nullptr);
    mvaddch(term_h_ - 1, x, x >= tooltip.size() ? ' ' : tooltip[x]);
  }

  hierarchy_->Draw();
  source_->Draw();
  waves_->Draw();
  wnoutrefresh(stdscr);
  wnoutrefresh(hierarchy_->Window());
  wnoutrefresh(source_->Window());
  wnoutrefresh(waves_->Window());
  doupdate();
}

} // namespace sv
