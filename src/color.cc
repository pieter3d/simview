#include "color.h"
#include <ncurses.h>

namespace sv {

namespace {
constexpr int kBackground = COLOR_BLACK;
}

void SetupColors() {
  if (has_colors()) {
    start_color();
    init_pair(kBorderPair, 8, kBackground);       // gray
    init_pair(kFocusBorderPair, 11, kBackground); // yellow
    init_pair(kOverflowTextPair, 9, kBackground); // bright red
    init_pair(kHierExpandPair, 10, kBackground);  // bright green
    init_pair(kHierInstancePair, COLOR_WHITE, kBackground);
    init_pair(kHierModulePair, COLOR_CYAN, kBackground);
    use_default_colors();
  }
}

} // namespace sv
