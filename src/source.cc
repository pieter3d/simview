#include "source.h"

namespace sv {

void Source::Draw() const {
  werase(w_);
  // test
  mvwprintw(w_, 0, 0, "Beep!");
}

void Source::UIChar(int ch) {}

} // namespace sv
