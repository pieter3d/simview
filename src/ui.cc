#include "ui.h"

#include "color.h"
#include "slang/ast/Symbol.h"
#include "workspace.h"
#include <ncurses.h>

namespace sv {
namespace {
const Tooltip kHelpTT = {.hotkeys = "?", .description = "help"};
}

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
      wresize(wave_signals_panel_->Window(), wave_h, layout_.waves_x - layout_.signals_x - 1);
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

void UI::UpdateTooltips() {
  int term_w = getmaxx(stdscr);
  tooltips_.clear();
  // Add the tooltips that are always present.
  tooltips_.push_back({"C-q", "quit"});
  if (layout_.has_waves) {
    tooltips_.push_back(
        {.hotkeys = "C-p",
         .description =
             std::string(layout_.show_wave_picker ? "SHOW/hide" : "show/HIDE") + " picker"});
  }
  if (panels_[focused_panel_idx_]->Searchable()) {
    tooltips_.push_back({"/nN", "search"});
  }
  auto panel_tooltips = panels_[focused_panel_idx_]->Tooltips();
  tooltips_.insert(tooltips_.end(), panel_tooltips.begin(), panel_tooltips.end());
  tooltips_to_show = tooltips_.size();
  int tt_width = 0;
  for (auto &tt : tooltips_) {
    tt_width += tt.Width() + 1;
  }
  tt_width--; // no trailing space.
  const int help_width = kHelpTT.Width();
  if (tt_width > term_w) {
    while ((tt_width + 1 + help_width) > term_w && tooltips_to_show > 0) {
      tt_width -= tooltips_[tooltips_to_show - 1].Width() + 1;
      tooltips_to_show--;
    }
  }
}

void UI::CycleFocus(bool fwd) {
  const int step = fwd ? 1 : -1;
  panels_[focused_panel_idx_]->SetFocus(false);
  while (1) {
    focused_panel_idx_ += step;
    if (focused_panel_idx_ < 0) focused_panel_idx_ = panels_.size() - 1;
    if (focused_panel_idx_ >= panels_.size()) focused_panel_idx_ = 0;
    if (layout_.show_wave_picker || (panels_[focused_panel_idx_] != wave_tree_panel_.get() &&
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
  nonl(); // don't translate the enter key
  SetupColors();
  // TODO - async needed?
  // halfdelay(3); // Update async things every 300ms.

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
    wave_tree_panel_ = std::make_unique<WaveDataTreePanel>();
    wave_signals_panel_ = std::make_unique<WaveSignalsPanel>();
    waves_panel_ = std::make_unique<WavesPanel>();
    panels_.push_back(wave_tree_panel_.get());
    panels_.push_back(wave_signals_panel_.get());
    panels_.push_back(waves_panel_.get());
  }

  if (!panels_.empty()) panels_[focused_panel_idx_]->SetFocus(true);

  // Initial render
  UpdateTooltips();
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

    // Any key clears an error message.
    if (ch != ERR) {
      error_message_.clear();
    }

    if (ch == ERR) {
      // TODO: Update async things?
    } else if (ch == KEY_RESIZE) {
      CalcLayout();
      LayoutPanels();
      UpdateTooltips();
    } else if (draw_tooltips_) {
      draw_tooltips_ = false;
    } else if (searching_) {
      // Searching is modal, so do nothing else until that is handled.
      const auto search_state = search_box_.HandleKey(ch);
      searching_ = search_state == TextInput::kTyping;
    } else {
      int term_w, term_h;
      getmaxyx(stdscr, term_h, term_w);
      // Normal mode. Top UI only really handles pane resizing and some search.
      auto *focused_panel = panels_[focused_panel_idx_];
      if (focused_panel->Modal()) {
        focused_panel->UIChar(ch);
      } else {
        switch (ch) {
        case 0x8: // ctrl-H
          if (focused_panel == design_tree_panel_.get() || focused_panel == source_panel_.get()) {
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
        case 0xc: // ctrl-L
          if (focused_panel == design_tree_panel_.get() || focused_panel == source_panel_.get()) {
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
        case 0xb: // ctrl-K
          if (layout_.wave_y > 5) {
            layout_.wave_y--;
            CalcLayout(true);
            LayoutPanels();
          }
          break;
        case 0xa: // ctrl-J
          if (layout_.wave_y < term_h - 5) {
            layout_.wave_y++;
            CalcLayout(true);
            LayoutPanels();
          }
          break;
          break;
        case 0x10: // ctrl-P
          layout_.show_wave_picker = !layout_.show_wave_picker;
          // If one of those panels was selected, move focus to the wave panel.
          if (focused_panel == wave_tree_panel_.get() || wave_signals_panel_.get()) {
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
          UpdateTooltips();
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
          quit = true;
          break;
        case '?':
          if (tooltips_to_show < tooltips_.size()) draw_tooltips_ = true;
          break;
        default:
          focused_panel->UIChar(ch);
          if (focused_panel->TooltipsChanged()) UpdateTooltips();
          break;
        }
      }
      // Look for transfers between panels
      if (focused_panel == design_tree_panel_.get()) {
        if (std::optional<const slang::ast::Symbol *> sym = design_tree_panel_->ItemForSource()) {
          source_panel_->SetItem(*sym);
        }
      } else if (focused_panel == source_panel_.get()) {
        if (const auto item = source_panel_->ItemForDesignTree()) {
          design_tree_panel_->SetItem(*item);
        }
        if (std::optional<const slang::ast::Symbol *> item = source_panel_->ItemForWaves()) {
          std::vector<const WaveData::Signal *> signals = Workspace::Get().DesignToSignals(*item);
          if (signals.empty()) {
            error_message_ = absl::StrFormat("%s is not available in the waves.", (*item)->name);
          } else {
            waves_panel_->AddSignals(signals);
          }
        }
      } else if (focused_panel == wave_tree_panel_.get()) {
        if (const auto scope = wave_tree_panel_->ScopeForSignals()) {
          wave_signals_panel_->SetScope(*scope);
        }
      } else if (focused_panel == wave_signals_panel_.get()) {
        if (const auto signals = wave_signals_panel_->SignalsForWaves()) {
          waves_panel_->AddSignals(*signals);
        }
      } else if (focused_panel == waves_panel_.get()) {
        if (const auto signal = waves_panel_->SignalForSource()) {
          // TODO - reinstate when slang-ified.
          // auto design_item = Workspace::Get().SignalToDesign(*signal);
          // if (design_item == nullptr) {
          //  error_message_ =
          //      absl::StrFormat("Signal %s not found in design.",
          //      WaveData::SignalToPath(*signal));
          //} else {
          //  source_panel_->SetItem(design_item, /* show_def*/ true);
          //  design_tree_panel_->SetItem(design_item);
          //}
        }
      }
    }
    if (quit) break;
    Draw();
  }
}

void UI::DrawHelp(int panel_idx) const {
  WINDOW *w = panels_[panel_idx]->Window();
  // Clear old stuff in the window, it can show if the terminal is resized.
  werase(w);
  // Find the widest signal.
  size_t widest_key = 0;
  size_t widest_description = 0;
  for (auto &tt : tooltips_) {
    widest_key = std::max(widest_key, tt.hotkeys.size());
    widest_description = std::max(widest_description, tt.description.size());
  }
  // Width of the help box accounts for the colon and a left + right margin.
  const int widest_text = widest_key + widest_description + 3;
  const int max_w = getmaxx(w);
  const int max_h = getmaxy(w);
  const int x_start = std::max(0, max_w / 2 - widest_text / 2);
  const int x_stop = std::min(max_w - 1, max_w / 2 + widest_text / 2);
  const int y_start = std::max(0, max_h / 2 - (int)tooltips_.size() / 2);
  const int y_stop = std::min(max_h - 1, max_h / 2 + (int)tooltips_.size() / 2);
  const int max_tt_idx = std::min((int)tooltips_.size() - 1, y_stop - y_start);
  for (int y = 0; y <= max_tt_idx; ++y) {
    wmove(w, y_start + y, x_start);
    SetColor(w, kTooltipPair);
    waddch(w, ' ');
    SetColor(w, kTooltipKeyPair);
    const auto &tt = tooltips_[y];
    const bool overflow = y == (max_h - 1) && y < (tooltips_.size() - 1);
    for (int x = 0; x <= x_stop - x_start; ++x) {
      if (overflow) {
        static std::string more = "... more ...";
        int pos = x - (x_stop - x_start + 1 - more.size()) / 2;
        waddch(w, (pos < 0 || pos >= more.size()) ? ' ' : more[pos]);
      } else if (x < widest_key) {
        waddch(w, x < tt.hotkeys.size() ? tt.hotkeys[x] : ' ');
      } else if (x == widest_key) {
        SetColor(w, kTooltipPair);
        waddch(w, ':');
      } else {
        int pos = x - widest_key - 1;
        waddch(w, pos < tt.description.size() ? tt.description[pos] : ' ');
      }
    }
  }
}

void UI::Draw() const {
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
    const bool highlighted =
        focused_panel == design_tree_panel_.get() || focused_panel == source_panel_.get();
    SetColor(stdscr, highlighted ? kFocusBorderPair : kBorderPair);
    mvvline(0, layout_.src_x, ACS_VLINE, layout_.wave_y);
  }
  // Draw the verical lines between the three wave panels
  if (layout_.has_waves && layout_.show_wave_picker) {
    int start_y = layout_.has_design ? layout_.wave_y + 1 : 0;
    int line_h = layout_.has_design ? term_h - layout_.wave_y - 2 : term_h - 1;
    const bool highlight_left =
        focused_panel == wave_tree_panel_.get() || focused_panel == wave_signals_panel_.get();
    const bool highlight_right =
        focused_panel == wave_signals_panel_.get() || focused_panel == waves_panel_.get();
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
    // Render the tooltips when not searching.
    int tt_idx = 0;
    int tt_key_idx = 0;
    int tt_description_idx = 0;
    // If not all tooltips can be shown, include an extra index for the trailing
    // help tooltip.
    const int num_tt_to_draw = tooltips_to_show + (tooltips_to_show < tooltips_.size() ? 1 : 0);
    enum { KEY, COLON, DESCRIPTION, SPACE } tt_state = KEY;
    SetColor(stdscr, kTooltipKeyPair);
    for (int x = 0; x < term_w; ++x) {
      const auto &tt = tt_idx >= tooltips_to_show ? kHelpTT : tooltips_[tt_idx];
      switch (tt_state) {
      case KEY:
        mvaddch(term_h - 1, x, tt.hotkeys[tt_key_idx++]);
        if (tt_key_idx == tt.hotkeys.size()) {
          tt_key_idx = 0;
          SetColor(stdscr, kTooltipPair);
          tt_state = COLON;
        }
        break;
      case COLON:
        mvaddch(term_h - 1, x, ':');
        tt_state = DESCRIPTION;
        break;
      case DESCRIPTION:
        mvaddch(term_h - 1, x, tt.description[tt_description_idx++]);
        if (tt_description_idx == tt.description.size()) {
          tt_description_idx = 0;
          tt_idx++;
          tt_state = SPACE;
        }
        break;
      case SPACE:
        mvaddch(term_h - 1, x, ' ');
        if (tt_idx < num_tt_to_draw) {
          SetColor(stdscr, kTooltipKeyPair);
          tt_state = KEY;
        }
        break;
      }
    }
  }

  wnoutrefresh(stdscr);
  for (auto &p : panels_) {
    // Skip the two wave picker panels if they are hidden.
    if (!layout_.show_wave_picker &&
        (p == wave_tree_panel_.get() || p == wave_signals_panel_.get())) {
      continue;
    }
    if (draw_tooltips_ && p == panels_[focused_panel_idx_]) {
      DrawHelp(focused_panel_idx_);
    } else {
      p->Draw();
    }
    wnoutrefresh(p->Window());
  }
  // Update cursor position, if there is one.
  if (searching_) {
    auto [row, col] = search_box_.CursorPos();
    move(row, col);
    curs_set(1);
  } else {
    if (focused_panel->CursorLocation()) {
      const auto [row, col] = *focused_panel->CursorLocation();
      int x, y;
      getbegyx(focused_panel->Window(), y, x);
      move(row + y, col + x);
      curs_set(1);
    } else {
      curs_set(0);
    }
  }
  // Finally, render an error message if needed.
  const std::string err = error_message_.empty() ? focused_panel->Error() : error_message_;
  if (!err.empty()) {
    move(term_h - 1, std::max(0ul, term_w - err.size()));
    SetColor(stdscr, kPanelErrorPair);
    addnstr(err.c_str(), term_w);
  }
  doupdate();
}

} // namespace sv
