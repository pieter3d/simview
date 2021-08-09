#include "panel.h"

namespace sv {

Panel::Panel(int h, int w, int row, int col) { w_ = newwin(h, w, row, col); }

std::pair<int, int> Panel::ScrollArea() {
  int h, w;
  getmaxyx(w_, h, w);
  return {h, w};
}

std::optional<std::pair<int, int>> Panel::CursorLocation() const {
  return std::nullopt;
}

void Panel::UIChar(int ch) {
  auto [max_h, max_w] = ScrollArea();
  switch (ch) {
  case 'h':
  case 0x104: // left
    break;
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
  case 0x153: { // PgUp
    if (scroll_row_ == 0) break;
    scroll_row_ -= std::min(scroll_row_, max_h - 2);
    if (line_idx_ - scroll_row_ >= max_h) {
      line_idx_ = scroll_row_ + max_h - 2;
    }
    break;
  }
  case 0x152: { // PgDn
    if (NumLines() <= max_h) break;
    scroll_row_ += std::min(max_h - 2, NumLines() - (scroll_row_ + max_h));
    if (line_idx_ - scroll_row_ < 0) {
      line_idx_ = scroll_row_ + 1;
    }
    break;
  }
  case 'g':   // vim style
  case 0x217: // Ctrl Home
    scroll_row_ = 0;
    line_idx_ = 0;
    break;
  case 'G':   // vim style
  case 0x212: // Ctrl End
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

void Panel::SetLineAndScroll(int l) {
  // Scroll to the selected line, attempt to place the line at 1/3rd the
  // screen.
  line_idx_ = l;
  const int win_h = ScrollArea().first;
  const int lines_remaining = NumLines() - line_idx_ - 1;
  if (NumLines() <= win_h - 1) {
    // If all lines fit on the screen, accounting for the header, then just
    // don't scroll.
    scroll_row_ = 0;
  } else if (line_idx_ < win_h / 3) {
    // Go as far down to the 1/3rd line as possible.
    scroll_row_ = 0;
  } else if (lines_remaining < 2 * win_h / 3) {
    // If there are aren't many lines after the current location, scroll as
    // far up as possible.
    scroll_row_ = NumLines() - win_h;
  } else {
    scroll_row_ = line_idx_ - win_h / 3;
  }
}

} // namespace sv
