#include "color.h"

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
    init_pair(kHierShowMorePair, COLOR_YELLOW, kBackground);
    init_pair(kHierOtherPair, 13, kBackground);          // Bright magenta
    init_pair(kTooltipPair, COLOR_BLACK, COLOR_YELLOW);  // Yellow background
    init_pair(kTooltipKeyPair, COLOR_RED, COLOR_YELLOW); // Yellow background
    init_pair(kSourceLineNrPair, COLOR_GREEN, kBackground);
    init_pair(kSourceHeaderPair, 12, kBackground); // Bright blue
    init_pair(kSourceTextPair, COLOR_WHITE, kBackground);
    use_default_colors();
  }
}

void SetColor(WINDOW *w, int n) { wcolor_set(w, n, nullptr); }

} // namespace sv
