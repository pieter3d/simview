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

  getmaxyx(stdscr, term_h_, term_w_);
  search_box_.SetDims(term_h_ - 1, 0, term_w_);
  // Default splits on startup.
  src_pos_x_ = term_w_ * 30 / 100;

  // Create all UI panels.
  const bool has_design = Workspace::Get().Design() != nullptr;
  const bool has_waves = Workspace::Get().Waves() != nullptr;
  if (has_design && has_waves) {
    wave_pos_y_ = term_h_ / 2;
  } else if (has_design) {
    // Leave room for the tooltip
    wave_pos_y_ = term_h_ - 1;
  } else {
    wave_pos_y_ = -1;
  }
  if (has_design) {
    design_tree_ =
        std::make_unique<DesignTreePanel>(wave_pos_y_, src_pos_x_ - 1, 0, 0);
    source_ = std::make_unique<SourcePanel>(wave_pos_y_, term_w_ - src_pos_x_,
                                            0, src_pos_x_ + 1);
    panels_.push_back(design_tree_.get());
    panels_.push_back(source_.get());
  }
  if (has_waves) {
    waves_ = std::make_unique<WavesPanel>(term_h_ - wave_pos_y_ - 2, term_w_,
                                          wave_pos_y_ + 1, 0);
    panels_.push_back(waves_.get());
  }

  if (!panels_.empty()) panels_[focused_panel_idx_]->SetFocus(true);

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
      const auto now = absl::Now();
      if (now - last_ch > absl::Milliseconds(50)) {
        tmp_ch.clear();
      }
      tmp_ch.push_back(ch);
      last_ch = now;
    }

    if (ch == ERR) {
      // TODO: Update async things?
    } else if (ch == KEY_RESIZE) {
      const float x = (float)src_pos_x_ / term_w_;
      const float y = (float)wave_pos_y_ / term_h_;
      getmaxyx(stdscr, term_h_, term_w_);
      src_pos_x_ = (int)(term_w_ * x);
      wave_pos_y_ = (int)(term_h_ * y);
      if (searching_) {
        search_box_.SetDims(term_h_ - 1, 0, term_w_);
      }
      resize = true;
    } else if (searching_) {
      // Searching is modal, so do nothing else until that is handled.
      const auto search_state = search_box_.HandleKey(ch);
      searching_ = search_state == TextInput::kTyping;
    } else {
      // Normal mode. Top UI only really handles pane resizing and some search.
      auto *focused_panel = panels_[focused_panel_idx_];
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
        if (waves_ == nullptr) break;
        if (wave_pos_y_ > 5) {
          wave_pos_y_--;
          resize = true;
        }
        break;
      case 0xa:   // ctrl-J
      case 0x20d: // ctrl-down
        if (waves_ == nullptr) break;
        if (wave_pos_y_ < term_h_ - 5) {
          wave_pos_y_++;
          resize = true;
        }
        break;
        break;
      case 0x9:     // tab
      case 0x161: { // shift-tab
        const bool fwd = ch == 0x9;
        panels_[focused_panel_idx_]->SetFocus(false);
        focused_panel_idx_ = (fwd ? (focused_panel_idx_ + 1)
                                  : (focused_panel_idx_ + panels_.size() - 1)) %
                             panels_.size();
        panels_[focused_panel_idx_]->SetFocus(true);
      } break;
      case '/':
        if (focused_panel->Searchable()) {
          searching_ = true;
          search_box_.SetReceiver(focused_panel);
          search_box_.Reset();
        }
        break;
      case 'n': focused_panel->Search(/*search_down*/ true); break;
      case 'N': focused_panel->Search(/*search_down*/ false); break;
      case 0x3:  // Ctrl-C
      case 0x11: // Ctrl-Q
      case 'q':  // TODO For now, remove eventually.
        quit = true;
        break;
      default: focused_panel->UIChar(ch); break;
      }
      // Look for transfers between panels
      if (focused_panel == design_tree_.get()) {
        if (auto item = design_tree_->ItemForSource()) {
          source_->SetItem(item->first, item->second);
        }
      } else if (focused_panel == source_.get()) {
        if (const auto item = source_->ItemForDesignTree()) {
          design_tree_->SetItem(*item);
        }
      }
    }
    if (quit) break;
    DrawPanes(resize);
  }
}

void UI::DrawPanes(bool resize) {
  const bool has_design = design_tree_ != nullptr;
  const bool has_waves = waves_ != nullptr;
  if (resize) {
    if (has_design) {
      wresize(design_tree_->Window(), wave_pos_y_, src_pos_x_);
      wresize(source_->Window(), wave_pos_y_, term_w_ - src_pos_x_ - 1);
      mvwin(source_->Window(), 0, src_pos_x_ + 1);
    }
    if (has_waves) {
      wresize(waves_->Window(), term_h_ - wave_pos_y_ - 2, term_w_);
      mvwin(waves_->Window(), wave_pos_y_ + 1, 0);
    }
    for (auto &p : panels_) {
      p->Resized();
    }
  }
  erase();
  const auto *focused_panel = panels_[focused_panel_idx_];
  // Render the dividing lines
  if (!has_waves) {
    // In the case of no waves, there is only the vertical dividing line. Use
    // the TMux style method to indicate which side is highlighted: Upper half
    // indicates left, lower half indicates right.
    const bool upper_highlighted = focused_panel == design_tree_.get();
    SetColor(stdscr, upper_highlighted ? kFocusBorderPair : kBorderPair);
    mvvline(0, src_pos_x_, ACS_VLINE, term_h_ / 2);
    SetColor(stdscr, !upper_highlighted ? kFocusBorderPair : kBorderPair);
    mvvline(term_h_ / 2, src_pos_x_, ACS_VLINE, term_h_);
  } else if (!has_design) {
    // TODO: Draw wave dividers.
  } else {
    // Both wave and design.
    // TODO: Redo this...
    if (focused_panel != waves_.get()) {
      SetColor(stdscr, kFocusBorderPair);
    } else {
      SetColor(stdscr, kBorderPair);
    }
    mvvline(0, src_pos_x_, ACS_VLINE, wave_pos_y_);
    if (focused_panel != source_.get()) {
      SetColor(stdscr, kFocusBorderPair);
    } else {
      SetColor(stdscr, kBorderPair);
    }
    mvhline(wave_pos_y_, 0, ACS_HLINE, src_pos_x_);
    SetColor(stdscr, kFocusBorderPair);
    mvaddch(wave_pos_y_, src_pos_x_, ACS_BTEE);
    if (focused_panel != design_tree_.get()) {
      SetColor(stdscr, kFocusBorderPair);
    } else {
      SetColor(stdscr, kBorderPair);
    }
    hline(ACS_HLINE, term_w_ - src_pos_x_ - 1);
  }

  if (searching_) {
    search_box_.Draw(stdscr);
  } else {
    // Render the tooltip when not searching.
    auto tooltip = "C-q:quit  /nN:search  " + focused_panel->Tooltip();
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

    // Display most recent key codes for development ease. TODO: remove
    std::string s;
    for (int code : tmp_ch) {
      s.append(absl::StrFormat("0x%x ", code));
    }
    SetColor(stdscr, kFocusBorderPair);
    mvprintw(term_h_ - 1, 2 * term_w_ / 3, "codes: %s", s.c_str());
  }

  wnoutrefresh(stdscr);
  for (auto &p : panels_) {
    p->Draw();
    wnoutrefresh(p->Window());
  }
  // Update cursor position, if there is one.
  if (searching_) {
    auto loc = search_box_.CursorPos();
    move(loc.first, loc.second);
    curs_set(1);
  } else {
    if (auto loc = focused_panel->CursorLocation()) {
      int x, y;
      getbegyx(focused_panel->Window(), y, x);
      move(loc->first + y, loc->second + x);
      curs_set(1);
    } else {
      curs_set(0);
    }
  }
  doupdate();
}

} // namespace sv
