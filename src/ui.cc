#include "ui.h"

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "color.h"
#include <ncurses.h>

namespace sv {

UI::UI() : search_box_("/") {
  setlocale(LC_ALL, "");
  // Init ncurses
  initscr();
  raw(); // Capture ctrl-c etc.
  keypad(stdscr, true);
  noecho();
  nonl();       // don't translate the enter key
  halfdelay(3); // Update async things every 300ms.
  SetupColors();

  // Default splits on startup.
  getmaxyx(stdscr, term_h_, term_w_);
  src_pos_x_ = term_w_ * 30 / 100;
  wave_pos_y_ = term_h_ * 50 / 100;
  search_box_.SetDims(term_h_ - 1, 0, term_w_);

  // Create all UI panels.
  design_tree_ =
      std::make_unique<DesignTreePanel>(wave_pos_y_, src_pos_x_ - 1, 0, 0);
  source_ = std::make_unique<SourcePanel>(wave_pos_y_, term_w_ - src_pos_x_, 0,
                                          src_pos_x_ + 1);
  waves_ = std::make_unique<WavesPanel>(term_h_ - wave_pos_y_ - 2, term_w_,
                                        wave_pos_y_ + 1, 0);
  focused_panel_ = design_tree_.get();
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
  while (int ch = getch()) {
    bool resize = false;
    bool quit = false;

    // TODO: remove
    if (ch != ERR) {
      auto now = absl::Now();
      if (now - last_ch > absl::Milliseconds(50)) {
        tmp_ch.clear();
      }
      tmp_ch.push_back(ch);
      last_ch = now;
    }

    if (ch == ERR) {
      // TODO: Update async things?
    } else if (ch == KEY_RESIZE) {
      float x = (float)src_pos_x_ / term_w_;
      float y = (float)wave_pos_y_ / term_h_;
      getmaxyx(stdscr, term_h_, term_w_);
      src_pos_x_ = (int)(term_w_ * x);
      wave_pos_y_ = (int)(term_h_ * y);
      if (searching_) {
        search_box_.SetDims(term_h_ - 1, 0, term_w_);
      }
      resize = true;
    } else if (searching_) {
      // Searching is modal, so do nothing else until that is handled.
      auto search_state = search_box_.HandleKey(ch);
      searching_ = search_state == TextInput::kTyping;
    } else {
      // Normal mode. Top UI only really handles pane resizing.
      switch (ch) {
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
      case 0x189: // shift-left
        if (focused_panel_ == source_.get()) {
          prev_focused_panel_ = focused_panel_;
          focused_panel_ = design_tree_.get();
        }
        break;
      case 0x192: // shift-right
        if (focused_panel_ == design_tree_.get()) {
          prev_focused_panel_ = focused_panel_;
          focused_panel_ = source_.get();
        }
        break;
      case 0x151: // shift-up
        if (focused_panel_ == waves_.get()) {
          focused_panel_ = prev_focused_panel_;
          prev_focused_panel_ = waves_.get();
        }
        break;
      case 0x150: // shift-down
        if (focused_panel_ != waves_.get()) {
          prev_focused_panel_ = focused_panel_;
          focused_panel_ = waves_.get();
        }
        break;
      case 0x9: // tab
        prev_focused_panel_ = focused_panel_;
        if (focused_panel_ == design_tree_.get()) {
          focused_panel_ = source_.get();
        } else if (focused_panel_ == source_.get()) {
          focused_panel_ = waves_.get();
        } else if (focused_panel_ == waves_.get()) {
          focused_panel_ = design_tree_.get();
        }
        break;
      case 0x161: // shift-tab
        prev_focused_panel_ = focused_panel_;
        if (focused_panel_ == design_tree_.get()) {
          focused_panel_ = waves_.get();
        } else if (focused_panel_ == source_.get()) {
          focused_panel_ = design_tree_.get();
        } else if (focused_panel_ == waves_.get()) {
          focused_panel_ = source_.get();
        }
        break;
      case '/':
        searching_ = true;
        search_box_.SetReceiver(focused_panel_);
        search_box_.Reset();
        break;
      case 'n': focused_panel_->Search(/*search_down*/ true); break;
      case 'N': focused_panel_->Search(/*search_down*/ false); break;
      case 0x3:  // Ctrl-C
      case 0x11: // Ctrl-Q
      case 'q':  // TODO For now, remove eventuall.
        quit = true;
        break;
      default: focused_panel_->UIChar(ch); break;
      }
    }
    if (quit) break;
    // Update focus state.
    prev_focused_panel_->SetFocus(false);
    focused_panel_->SetFocus(true);
    // Look for transfers from DesignTree -> Source
    if (focused_panel_ == design_tree_.get()) {
      if (auto item = design_tree_->ItemForSource()) {
        source_->SetItem(item->first, item->second);
      }
    } else if (focused_panel_ == source_.get()) {
      if (auto item = source_->ItemForDesignTree()) {
        design_tree_->SetItem(*item);
      }
    }
    DrawPanes(resize);
  }
}

void UI::DrawPanes(bool resize) {
  if (resize) {
    wresize(design_tree_->Window(), wave_pos_y_, src_pos_x_);
    wresize(source_->Window(), wave_pos_y_, term_w_ - src_pos_x_ - 1);
    mvwin(source_->Window(), 0, src_pos_x_ + 1);
    wresize(waves_->Window(), term_h_ - wave_pos_y_ - 2, term_w_);
    mvwin(waves_->Window(), wave_pos_y_ + 1, 0);
    design_tree_->Resized();
    source_->Resized();
    waves_->Resized();
  }
  erase();
  // Render the dividing lines
  if (focused_panel_ != waves_.get()) {
    SetColor(stdscr, kFocusBorderPair);
  } else {
    SetColor(stdscr, kBorderPair);
  }
  mvvline(0, src_pos_x_, ACS_VLINE, wave_pos_y_);
  if (focused_panel_ != source_.get()) {
    SetColor(stdscr, kFocusBorderPair);
  } else {
    SetColor(stdscr, kBorderPair);
  }
  mvhline(wave_pos_y_, 0, ACS_HLINE, src_pos_x_);
  SetColor(stdscr, kFocusBorderPair);
  mvaddch(wave_pos_y_, src_pos_x_, ACS_BTEE);
  if (focused_panel_ != design_tree_.get()) {
    SetColor(stdscr, kFocusBorderPair);
  } else {
    SetColor(stdscr, kBorderPair);
  }
  hline(ACS_HLINE, term_w_ - src_pos_x_ - 1);

  // Display most recent key codes for development ease. TODO: remove
  std::string s;
  for (int code : tmp_ch)
    s.append(absl::StrFormat("0x%x ", code));
  mvprintw(wave_pos_y_, 0, "codes: %s", s.c_str());

  if (searching_) {
    search_box_.Draw(stdscr);
  } else {
    // Render the tooltip when not searching.
    auto tooltip = "C-q:quit  /nN:search  " + focused_panel_->Tooltip();
    for (int x = 0; x < term_w_; ++x) {
      // Look for a key (indicated by colon following).
      bool is_key = false;
      for (int i = x + 1; i < tooltip.size(); ++i) {
        if (tooltip[i] == ':') {
          is_key = true;
          break;
        } else if (tooltip[i] == ' ') {
          is_key = false;
          break;
        }
      }
      SetColor(stdscr, is_key ? kTooltipKeyPair : kTooltipPair);
      mvaddch(term_h_ - 1, x, x >= tooltip.size() ? ' ' : tooltip[x]);
    }
  }

  design_tree_->Draw();
  source_->Draw();
  waves_->Draw();
  wnoutrefresh(stdscr);
  wnoutrefresh(design_tree_->Window());
  wnoutrefresh(source_->Window());
  wnoutrefresh(waves_->Window());
  // Update cursor position, if there is one.
  if (searching_) {
    auto loc = search_box_.CursorPos();
    move(loc.first, loc.second);
    curs_set(1);
  } else {
    if (auto loc = focused_panel_->CursorLocation()) {
      int x, y;
      getbegyx(focused_panel_->Window(), y, x);
      move(loc->first + y, loc->second + x);
      curs_set(1);
    } else {
      curs_set(0);
    }
  }
  doupdate();
}

} // namespace sv
