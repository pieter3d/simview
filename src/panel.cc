#include "panel.h"

namespace sv {

std::pair<int, int> Panel::ScrollArea() {
  int h, w;
  getmaxyx(w_, h, w);
  return {h, w};
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

} // namespace sv
