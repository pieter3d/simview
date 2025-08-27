#include "panel.h"

namespace sv {

Panel::Panel() { w_ = newwin(1, 1, 0, 0); }

std::pair<int, int> Panel::ScrollArea() const {
  int h, w;
  getmaxyx(w_, h, w);
  return {h, w};
}

std::optional<std::pair<int, int>> Panel::CursorLocation() const { return std::nullopt; }

void Panel::UIChar(int ch) {
  auto [max_h, max_w] = ScrollArea();
  switch (ch) {
  case 'h':
  case 0x104: // left
  case 'l':
  case 0x105: // right
    break;
  case 'k':
  case 0x103: // up
    if (line_idx_ == 0) break;
    line_idx_--;
    if (line_idx_ - scroll_row_ < 0) scroll_row_--;
    break;
  case 'j':
  case 0x102: // down
    if (line_idx_ >= NumLines() - 1) break;
    line_idx_++;
    if (line_idx_ - scroll_row_ >= max_h) scroll_row_++;
    break;
  case 0x15:    // Ctrl-U
  case 0x153: { // PgUp
    if (scroll_row_ == 0) {
      // Back to the top if there's nowhere left to scroll up.
      line_idx_ = 0;
      break;
    }
    const int scroll_decrement = std::min(scroll_row_, max_h - 2);
    scroll_row_ -= scroll_decrement;
    if (line_idx_ - scroll_row_ >= max_h) {
      line_idx_ = scroll_row_ + max_h - 2;
    }
    break;
  }
  case 0x4:     // Ctrl-D
  case 0x152: { // PgDn
    if (NumLines() <= max_h) {
      line_idx_ = NumLines() - 1;
      break;
    }
    const int scroll_increment = std::min(max_h - 2, NumLines() - (scroll_row_ + max_h));
    scroll_row_ += scroll_increment;
    if (line_idx_ - scroll_row_ < 0) {
      line_idx_ = scroll_row_ + 1;
    }
    // Move the selected to the end if there's nowhere left to scroll down.
    if (scroll_increment == 0) {
      line_idx_ = NumLines() - 1;
    }
    break;
  }
  case 'g':   // vim style
  case 0x23f: // Ctrl Up
  case 0x220: // Ctrl Home
    scroll_row_ = 0;
    line_idx_ = 0;
    break;
  case 'G':   // vim style
  case 0x216: // Ctrl Down
  case 0x21b: // Ctrl End
    line_idx_ = NumLines() - 1;
    if (NumLines() > max_h) {
      scroll_row_ = NumLines() - max_h;
    }
    break;
  }
}

void Panel::Resized() {
  auto [max_h, max_w] = ScrollArea();
  if (line_idx_ - scroll_row_ >= max_h) {
    scroll_row_ = line_idx_ - max_h + 1;
  }
}

bool Panel::ReceiveText(const std::string &s, bool preview) {
  search_preview_ = preview;
  search_text_ = s;

  // Search only when previewing. A non-preview search can just leave things at the most recent
  // preview location.
  if (preview) {
    // Save the current line index so it can be restored if the search is cancelled.
    if (search_orig_line_idx_ < 0) {
      search_orig_line_idx_ = line_idx_;
      search_orig_col_idx_ = col_idx_;
    }
    const bool found = Search(true); // Search down
    if (!found) {
      SetLineAndScroll(search_orig_line_idx_);
      col_idx_ = search_orig_col_idx_;
    }
    return found;
  } else {
    if (search_text_.empty() && search_orig_col_idx_ >= 0) {
      SetLineAndScroll(search_orig_line_idx_);
      col_idx_ = search_orig_col_idx_;
    }
    search_orig_line_idx_ = -1;
    return true; // Doesn't actually matter here, search box is closed.
  }
}

void Panel::SetLineAndScroll(int l) {
  // Scroll to the selected line, attempt to place the line at 1/3rd the screen if the chosen line
  // is too close to the top or bottom.
  line_idx_ = l;
  const int win_h = ScrollArea().first;
  const int lines_remaining = NumLines() - line_idx_ - 1;
  if ((line_idx_ - scroll_row_) > 2 && (line_idx_ - scroll_row_ < win_h - 3)) return;
  if ((NumLines() <= win_h - 1) || (line_idx_ < win_h / 3)) {
    // If all lines fit on the screen, accounting for the header, then just don't scroll. Also if
    // scrolling to near the top, ensure the lines above are visible.
    scroll_row_ = 0;
  } else if (lines_remaining < 2 * win_h / 3) {
    // If there are aren't many lines after the current location, scroll as
    // far up as possible.
    scroll_row_ = std::max(0, NumLines() - win_h);
  } else {
    scroll_row_ = std::max(0, line_idx_ - win_h / 3);
  }
}

bool Panel::TooltipsChanged() {
  bool b = tooltips_changed_;
  tooltips_changed_ = false;
  return b;
}

std::string Panel::Error() const {
  std::string error = error_message_;
  error_message_.clear();
  return error;
}

} // namespace sv
