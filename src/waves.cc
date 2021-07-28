#include "waves.h"

namespace sv {

void Waves::Draw() {
  werase(w_);
  // test
  mvwprintw(w_, 0, 0, "Braap!");
}

void Waves::UIChar(int ch) {}

bool Waves::TransferPending() { return false; }

} // namespace sv
