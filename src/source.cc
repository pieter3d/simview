#include "source.h"
#include "color.h"
#include <curses.h>
#include <fstream>

namespace sv {
namespace {
// Remove newline characters from the start or end of the string.
void trim_string(std::string &s) {
  if (s.empty()) return;
  if (s.size() > 1 && s[s.size() - 2] == '\r' && s[s.size() - 1] == '\n') {
    s.erase(s.size() - 2, 1);
  }
  if (s[s.size() - 1] == '\n') s.erase(s.size() - 1, 1);
}

int num_decimal_digits(int n) {
  int ret = 0;
  do {
    ret++;
    n /= 10;
  } while (n != 0);
  return ret;
}
} // namespace

void Source::Draw() {
  werase(w_);
  wattrset(w_, A_NORMAL);
  if (state_.instance == nullptr) {
    mvwprintw(w_, 0, 0, "Module instance source code appears here.");
    return;
  }
  if (lines_.empty()) {
    mvwprintw(w_, 0, 0, "Unable to open file:");
    mvwprintw(w_, 1, 0, state_.instance->getFileName().c_str());
    return;
  }
  const int win_h = getmaxy(w_);
  const int win_w = getmaxx(w_);
  const int max_digits = num_decimal_digits(ui_row_scroll_ + win_h);

  for (int y = ui_row_scroll_; y < win_h + ui_row_scroll_; ++y) {
    if (y >= lines_.size()) break;
    const int line_num = ui_row_scroll_ + y + 1;
    const int line_num_size = num_decimal_digits(line_num);
    SetColor(w_, kSourceLineNrPair);
    mvwprintw(w_, y, max_digits - line_num_size, "%d", line_num);
    SetColor(w_, kSourceTextPair);
    waddch(w_, ' ');
    waddnstr(w_, lines_[y].text.c_str(), win_w - max_digits - 1);
  }
}

void Source::UIChar(int ch) {}

bool Source::TransferPending() { return false; }

void Source::SetInstance(SURELOG::ModuleInstance *inst) {
  state_.instance = inst;
  // Read all lines. TODO: Handle huge files.
  lines_.clear();
  std::ifstream is(inst->getFileName());
  if (is.fail()) return; // Draw function handles file open issues.
  while (is) {
    lines_.push_back({});
    auto &s = lines_.back().text;
    std::getline(is, s);
    trim_string(s);
  }
  // Scroll to module definition
  ui_line_index_ = 0;
  ui_col_scroll_ = 0;
  ui_row_scroll_ = state_.line_num;
}

} // namespace sv
