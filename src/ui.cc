#include "ui.h"

#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "color.h"
#include <ncurses.h>

namespace sv {

void UI::CalcLayout(bool update_frac) {
  int tw, th;
  getmaxyx(stdscr, th, tw);
  if (update_frac) {
    layout_.f_src_x = (float)layout_.src_x / tw;
    layout_.f_signals_x = (float)layout_.signals_x / tw;
    layout_.f_waves_x = (float)layout_.waves_x / tw;
    layout_.f_wave_y = (float)layout_.wave_y / th;
  } else {
    layout_.src_x = (int)(layout_.f_src_x * tw);
    layout_.signals_x = (int)(layout_.f_signals_x * tw);
    layout_.waves_x = (int)(layout_.f_waves_x * tw);
    layout_.wave_y = (int)(layout_.f_wave_y * th);
  }
}

void UI::LayoutPanels() {
  int tw, th;
  getmaxyx(stdscr, th, tw);
  if (layout_.has_design) {
    const int design_h = layout_.has_waves ? layout_.wave_y : th - 1;
    wresize(design_tree_panel_->Window(), design_h, layout_.src_x);
    wresize(source_panel_->Window(), design_h, tw - layout_.src_x - 1);
    mvwin(source_panel_->Window(), 0, layout_.src_x + 1);
  }
  if (layout_.has_waves) {
    const int wave_h = layout_.has_design ? (th - layout_.wave_y - 2) : th - 1;
    const int wave_y = layout_.has_design ? layout_.wave_y + 1 : 0;
    if (!layout_.show_wave_picker) {
      wresize(waves_panel_->Window(), wave_h, tw);
      mvwin(waves_panel_->Window(), wave_y, 0);
    } else {
      wresize(wave_tree_panel_->Window(), wave_h, layout_.signals_x);
      mvwin(wave_tree_panel_->Window(), wave_y, 0);
      wresize(wave_signals_panel_->Window(), wave_h,
              layout_.waves_x - layout_.signals_x - 1);
      mvwin(wave_signals_panel_->Window(), wave_y, layout_.signals_x + 1);
      wresize(waves_panel_->Window(), wave_h, tw - layout_.waves_x - 1);
      mvwin(waves_panel_->Window(), wave_y, layout_.waves_x + 1);
    }
  }
  // Notify all panels of the size change.
  for (auto &p : panels_) {
    p->Resized();
  }
  // Make the search box fit the bottom width of the screen.
  search_box_.SetDims(th - 1, 0, tw);
}

void UI::CycleFocus(bool fwd) {
  const int step = fwd ? 1 : -1;
  panels_[focused_panel_idx_]->SetFocus(false);
  while (1) {
    focused_panel_idx_ += step;
    if (focused_panel_idx_ < 0) focused_panel_idx_ = panels_.size() - 1;
    if (focused_panel_idx_ >= panels_.size()) focused_panel_idx_ = 0;
    if (layout_.show_wave_picker ||
        (panels_[focused_panel_idx_] != wave_tree_panel_.get() &&
         panels_[focused_panel_idx_] != wave_signals_panel_.get())) {
      break;
    }
  }
  panels_[focused_panel_idx_]->SetFocus(true);
}

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

  // Create all UI panels.
  layout_.has_design = Workspace::Get().Design() != nullptr;
  layout_.has_waves = Workspace::Get().Waves() != nullptr;
  // Create all panels with tiny sizes at first.
  if (layout_.has_design) {
    // Default splits on startup.
    design_tree_panel_ = std::make_unique<DesignTreePanel>();
    source_panel_ = std::make_unique<SourcePanel>();
    panels_.push_back(design_tree_panel_.get());
    panels_.push_back(source_panel_.get());
  }
  if (layout_.has_waves) {
    wave_tree_panel_ = std::make_unique<WaveTreePanel>();
    wave_signals_panel_ = std::make_unique<WavesPanel>();
    waves_panel_ = std::make_unique<WavesPanel>();
    panels_.push_back(wave_tree_panel_.get());
    panels_.push_back(wave_signals_panel_.get());
    panels_.push_back(waves_panel_.get());
  }

  if (!panels_.empty()) panels_[focused_panel_idx_]->SetFocus(true);

  // Initial render
  CalcLayout();
  LayoutPanels();
  Draw();
}

UI::~UI() {
  // Cleanup ncurses
  endwin();
}

void UI::EventLoop() {
  while (int ch = getch()) {
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
      CalcLayout();
      LayoutPanels();
    } else if (searching_) {
      // Searching is modal, so do nothing else until that is handled.
      const auto search_state = search_box_.HandleKey(ch);
      searching_ = search_state == TextInput::kTyping;
    } else {
      int term_w, term_h;
      getmaxyx(stdscr, term_h, term_w);
      // Normal mode. Top UI only really handles pane resizing and some search.
      auto *focused_panel = panels_[focused_panel_idx_];
      switch (ch) {
      case 0x8:   // ctrl-H
      case 0x221: // ctrl-left
        if (focused_panel == design_tree_panel_.get() ||
            focused_panel == source_panel_.get()) {
          // Keep some reasonable minimum size.
          if (layout_.src_x > 5) {
            layout_.src_x--;
          }
        } else if (layout_.show_wave_picker) {
          if (focused_panel == waves_panel_.get()) {
            if (layout_.waves_x > 10) {
              if (layout_.waves_x - layout_.signals_x <= 5) {
                layout_.signals_x--;
              }
              layout_.waves_x--;
            }
          } else {
            if (layout_.signals_x > 5) {
              layout_.signals_x--;
            }
          }
        }
        CalcLayout(true);
        LayoutPanels();
        break;
      case 0xc:   // ctrl-L
      case 0x230: // ctrl-right
        if (focused_panel == design_tree_panel_.get() ||
            focused_panel == source_panel_.get()) {
          if (layout_.src_x < term_w - 5) {
            layout_.src_x++;
          }
        } else if (layout_.show_wave_picker) {
          if (focused_panel == wave_tree_panel_.get() ||
              focused_panel == wave_signals_panel_.get()) {
            if (layout_.signals_x < term_w - 10) {
              if (layout_.waves_x - layout_.signals_x <= 5) {
                layout_.waves_x++;
              }
              layout_.signals_x++;
            }
          } else {
            if (layout_.waves_x < term_w - 5) {
              layout_.waves_x++;
            }
          }
        }
        CalcLayout(true);
        LayoutPanels();
        break;
      case 0xb:   // ctrl-K
      case 0x236: // ctrl-up
        if (layout_.wave_y > 5) {
          layout_.wave_y--;
          CalcLayout(true);
          LayoutPanels();
        }
        break;
      case 0xa:   // ctrl-J
      case 0x20d: // ctrl-down
        if (layout_.wave_y < term_h - 5) {
          layout_.wave_y++;
          CalcLayout(true);
          LayoutPanels();
        }
        break;
        break;
      case 'g':
        layout_.show_wave_picker = !layout_.show_wave_picker;
        // If one of those panels was selected, move focus to the wave panel.
        if (focused_panel == wave_tree_panel_.get() ||
            wave_signals_panel_.get()) {
          focused_panel->SetFocus(false);
          waves_panel_->SetFocus(true);
          focused_panel_idx_ = panels_.size() - 1;
        }
        LayoutPanels();
        break;
      case 0x9:     // tab
      case 0x161: { // shift-tab
        const bool fwd = ch == 0x9;
        CycleFocus(fwd);
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
      if (focused_panel == design_tree_panel_.get()) {
        if (auto item = design_tree_panel_->ItemForSource()) {
          source_panel_->SetItem(item->first, item->second);
        }
      } else if (focused_panel == source_panel_.get()) {
        if (const auto item = source_panel_->ItemForDesignTree()) {
          design_tree_panel_->SetItem(*item);
        }
      }
    }
    if (quit) break;
    Draw();
  }
}

void UI::Draw() {
  erase();
  const auto *focused_panel = panels_[focused_panel_idx_];
  // Render the dividing lines
  int term_w, term_h;
  getmaxyx(stdscr, term_h, term_w);
  // Draw the vertical line between the two design panels
  if (!layout_.has_waves) {
    // In the case of no waves, there is only the vertical dividing line. Use
    // the TMux style method to indicate which side is highlighted: Upper half
    // indicates left, lower half indicates right.
    const bool upper_highlighted = focused_panel == design_tree_panel_.get();
    SetColor(stdscr, upper_highlighted ? kFocusBorderPair : kBorderPair);
    mvvline(0, layout_.src_x, ACS_VLINE, term_h / 2);
    SetColor(stdscr, !upper_highlighted ? kFocusBorderPair : kBorderPair);
    mvvline(term_h / 2, layout_.src_x, ACS_VLINE, term_h);
  } else if (layout_.has_design) {
    // Both waves and design. always highlight the divider if one of the design
    // panels is focused.
    const bool highlighted = focused_panel == design_tree_panel_.get() ||
                             focused_panel == source_panel_.get();
    SetColor(stdscr, highlighted ? kFocusBorderPair : kBorderPair);
    mvvline(0, layout_.src_x, ACS_VLINE, layout_.wave_y);
  }
  // Draw the verical lines between the three wave panels
  if (layout_.has_waves && layout_.show_wave_picker) {
    int start_y = layout_.has_design ? layout_.wave_y + 1 : 0;
    int line_h = layout_.has_design ? term_h - layout_.wave_y - 2 : term_h - 1;
    const bool highlight_left = focused_panel == wave_tree_panel_.get() ||
                                focused_panel == wave_signals_panel_.get();
    const bool highlight_right = focused_panel == wave_signals_panel_.get() ||
                                 focused_panel == waves_panel_.get();
    SetColor(stdscr, highlight_left ? kFocusBorderPair : kBorderPair);
    mvvline(start_y, layout_.signals_x, ACS_VLINE, line_h);
    SetColor(stdscr, highlight_right ? kFocusBorderPair : kBorderPair);
    mvvline(start_y, layout_.waves_x, ACS_VLINE, line_h);
  }
  // Draw the horizontal divider
  if (layout_.has_design && layout_.has_waves) {
    int focus_start = 0;
    int focus_end = term_w - 1;
    if (focused_panel == design_tree_panel_.get()) {
      focus_start = 0;
      focus_end = layout_.src_x;
    } else if (focused_panel == source_panel_.get()) {
      focus_start = layout_.src_x;
      focus_end = term_w - 1;
    } else if (focused_panel == wave_tree_panel_.get()) {
      focus_start = 0;
      focus_end = layout_.signals_x;
    } else if (focused_panel == wave_signals_panel_.get()) {
      focus_start = layout_.signals_x;
      focus_end = layout_.waves_x;
    } else if (focused_panel == waves_panel_.get()) {
      focus_start = layout_.show_wave_picker ? layout_.waves_x : 0;
      focus_end = term_w - 1;
    }
    // Just go characater by character, too messy to find all contiguous
    // segments.
    wmove(stdscr, layout_.wave_y, 0);
    for (int i = 0; i < term_w; ++i) {
      bool at_design_split = i == layout_.src_x;
      bool at_picker_split = i == layout_.signals_x && layout_.show_wave_picker;
      bool at_waves_split = i == layout_.waves_x && layout_.show_wave_picker;
      bool highlight = i >= focus_start && i <= focus_end;
      SetColor(stdscr, highlight ? kFocusBorderPair : kBorderPair);
      if (at_design_split && (at_picker_split || at_waves_split)) {
        addch(ACS_PLUS);
      } else if (at_design_split) {
        addch(ACS_BTEE);
      } else if (at_picker_split || at_waves_split) {
        addch(ACS_TTEE);
      } else {
        addch(ACS_HLINE);
      }
    }
  }

  if (searching_) {
    search_box_.Draw(stdscr);
  } else {
    // Render the tooltip when not searching.
    std::string tooltip = "C-q:quit  /nN:search  ";
    if (layout_.has_waves) {
      tooltip += "g:";
      tooltip += layout_.show_wave_picker ? "SHOW/hide" : "show/HIDE";
      tooltip += " signals  ";
    }
    tooltip += focused_panel->Tooltip();
    for (int x = 0; x < term_w; ++x) {
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
      mvaddch(term_h - 1, x, x >= tooltip.size() ? ' ' : tooltip[x]);
    }

    // Display most recent key codes for development ease. TODO: remove
    std::string s;
    for (int code : tmp_ch) {
      s.append(absl::StrFormat("0x%x ", code));
    }
    SetColor(stdscr, kFocusBorderPair);
    mvprintw(term_h - 1, 2 * term_w / 3, "codes: %s", s.c_str());
  }

  wnoutrefresh(stdscr);
  for (auto &p : panels_) {
    // Skip the two wave picker panels if they are hidden.
    if (!layout_.show_wave_picker &&
        (p == wave_tree_panel_.get() || p == wave_signals_panel_.get())) {
      continue;
    }
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
