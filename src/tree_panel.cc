#include "tree_panel.h"
#include "color.h"

namespace sv {

void TreePanel::Draw() {
  werase(w_);
  int win_w, win_h;
  getmaxyx(w_, win_h, win_w);
  // If the window was resized and the ui line position is now hidden, move it
  // up.
  int max_string_len = 0;
  for (int y = header_lines_; y < win_h; ++y) {
    const int list_idx = y + scroll_row_ - header_lines_;
    if (list_idx >= data_.ListSize()) break;
    if (list_idx == line_idx_ && !search_preview_) {
      wattron(w_, has_focus_ ? A_REVERSE : A_UNDERLINE);
    }
    auto item = data_[list_idx];
    std::string indent(item->Depth(), ' ');
    if (item->MoreIdx() != 0) {
      SetColor(w_, kHierShowMorePair);
      mvwprintw(w_, y, 0, "%s...more...", indent.c_str());
    } else {
      std::string type_name = item->Type();
      std::string name = item->Name();
      char exp = item->Expandable() ? (item->Expanded() ? '-' : '+') : ' ';
      std::string s = indent;
      if (show_expanders_) s += exp;
      if (prepend_type_) {
        if (type_name.empty()) {
          s += name;
        } else {
          s += type_name + ' ' + name;
        }
      } else {
        s += name + ' ' + type_name;
      }
      max_string_len = std::max(max_string_len, static_cast<int>(s.size()));
      int expand_pos = indent.size();
      int text_pos = expand_pos + show_expanders_;
      int inst_pos =
          text_pos + (prepend_type_
                          ? (type_name.empty() ? 0 : (type_name.size() + 1))
                          : 0);
      int type_pos = text_pos + (prepend_type_ ? 0 : (name.size() + 1));
      const bool show_search =
          search_preview_ && list_idx == line_idx_ && search_start_col_ >= 0;
      const int search_pos = search_start_col_ + inst_pos;
      for (int j = 0; j < s.size(); ++j) {
        const int x = j - ui_col_scroll_;
        if (x < 0) continue;
        if (x >= win_w) break;
        if (x == 0 && ui_col_scroll_ != 0 && j >= expand_pos) {
          // Show an overflow character on the left edge if the ui has been
          // scrolled horizontally.
          SetColor(w_, kOverflowTextPair);
          mvwaddch(w_, y, x, '<');
        } else if (x == win_w - 1 && j < s.size() - 1) {
          // Replace the last character with an overflow indicator if the line
          // extends beyond the window width.
          SetColor(w_, kOverflowTextPair);
          mvwaddch(w_, y, x, '>');
        } else {
          if (j >= type_pos && j < type_pos + type_name.size()) {
            SetColor(w_, item->AltType() ? kHierOtherPair : kHierTypePair);
          } else if (j >= inst_pos) {
            if (show_search && j == search_pos) {
              wattron(w_, A_REVERSE);
            }
            SetColor(w_, kHierNamePair);
          } else if (j == expand_pos && item->Expandable()) {
            SetColor(w_, kHierExpandPair);
          }
          mvwaddch(w_, y, x, s[j]);
          if (show_search && j == search_pos + search_text_.size() - 1) {
            wattroff(w_, A_REVERSE);
          }
        }
      }
    }
    wattrset(w_, A_NORMAL);
  }
  ui_max_col_scroll_ = max_string_len - win_w;
}

void TreePanel::UIChar(int ch) {
  switch (ch) {
  case 'u': {
    int current_depth = data_[line_idx_]->Depth();
    if (current_depth != 0 && line_idx_ != 0) {
      while (data_[line_idx_]->Depth() >= current_depth) {
        line_idx_--;
      }
      if (line_idx_ - scroll_row_ < 0) {
        scroll_row_ = line_idx_;
      }
    }
    break;
  }
  case 0x20:  // space
  case 0xd: { // enter
    data_.ToggleExpand(line_idx_);
    // If the expanded data is all below, scroll up a ways.
    if (data_[line_idx_]->Expanded()) {
      const int win_h = getmaxy(w_);
      if (line_idx_ - scroll_row_ == win_h - 1) {
        const int lines_below = data_.ListSize() - scroll_row_ - win_h;
        const int scroll_amt = std::min(lines_below, win_h / 3);
        scroll_row_ += scroll_amt;
      }
    }
  } break;
  case 'h':
  case 0x104: // left
    if (ui_col_scroll_ > 0) ui_col_scroll_--;
    break;
  case 'l':
  case 0x105: // right
    if (ui_col_scroll_ < ui_max_col_scroll_) ui_col_scroll_++;
    break;
  case 'k':
  default: Panel::UIChar(ch);
  }
}

bool TreePanel::Search(bool search_down) {
  if (data_.Empty()) return false;
  int idx = line_idx_;
  const int start_idx = idx;
  const auto step = [&] {
    idx += search_down ? 1 : -1;
    if (idx < 0) {
      idx = data_.ListSize() - 1;
    } else if (idx >= data_.ListSize()) {
      idx = 0;
    }
  };
  while (1) {
    if (!search_preview_) step();
    const auto pos = data_[idx]->Name().find(search_text_, 0);
    if (pos != std::string::npos) {
      search_start_col_ = pos;
      SetLineAndScroll(idx);
      return true;
    }
    if (search_preview_) step();
    if (idx == start_idx) {
      search_start_col_ = -1;
      return false;
    }
  }
}

} // namespace sv
