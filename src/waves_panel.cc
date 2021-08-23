#include "waves_panel.h"

namespace sv {

void WavesPanel::Draw() {
  werase(w_);
  // test
  mvwprintw(w_, 0, 0, "Braap!");
}

void WavesPanel::UIChar(int ch) {}

} // namespace sv
