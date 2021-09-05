#include "color.h"

namespace sv {

namespace {
constexpr int kBackground = COLOR_BLACK;
}

void SetupColors() {
  if (has_colors()) {
    start_color();
    init_pair(kBorderPair, 8, kBackground);       // gray
    init_pair(kFocusBorderPair, 13, kBackground); // magenta
    init_pair(kOverflowTextPair, 9, kBackground); // bright red
    init_pair(kTextInputPair, 15, 5);             // white on purple
    init_pair(kTextInputRejectPair, 15, 1);       // white on red
    init_pair(kHierExpandPair, 10, kBackground);  // bright green
    init_pair(kHierNamePair, COLOR_WHITE, kBackground);
    init_pair(kHierTypePair, COLOR_CYAN, kBackground);
    init_pair(kHierShowMorePair, COLOR_YELLOW, kBackground);
    init_pair(kHierOtherPair, 13, kBackground);           // bright magenta
    init_pair(kTooltipPair, COLOR_BLACK, COLOR_YELLOW);   // yellow background
    init_pair(kTooltipKeyPair, COLOR_RED, COLOR_YELLOW);  // yellow background
    init_pair(kSourceLineNrPair, 8, kBackground);         // grey
    init_pair(kSourceCurrentLineNrPair, 11, kBackground); // yellow
    init_pair(kSourceHeaderPair, COLOR_CYAN, kBackground);
    init_pair(kSourceTextPair, 7, kBackground);       // silver
    init_pair(kSourceIdentifierPair, 9, kBackground); // red
    init_pair(kSourceKeywordPair, 11, kBackground);   // yellow
    init_pair(kSourceCommentPair, 12, kBackground);   // bright blue
    init_pair(kSourceInactivePair, 8, kBackground);   // grey
    init_pair(kSourceParamPair, 5, kBackground);      // purple
    init_pair(kSourceInstancePair, COLOR_CYAN, kBackground);
    init_pair(kSourceValuePair, 10, kBackground);     // bright green
    init_pair(kSignalToggleOnPair, 12, kBackground);  // bright blue
    init_pair(kSignalToggleOffPair, 8, kBackground);  // grey
    init_pair(kSignalFilterPair, 12, kBackground);    // bright blue
    init_pair(kWavesTimeTickPair, 12, kBackground);   // bright blue
    init_pair(kWavesTimeValuePair, 8, kBackground);   // grey
    init_pair(kWavesCursorPair, 11, kBackground);     // yellow
    init_pair(kWavesMarkerPair, 15, kBackground);     // white
    init_pair(kWavesDeltaPair, 6, kBackground);       // cyan
    init_pair(kWavesSignalNamePair, 15, kBackground); // white
    init_pair(kWavesSignalValuePair, 6, kBackground); // cyan
    init_pair(kWavesGroupPair, 10, kBackground);      // bright green
    init_pair(kWavesWaveformPair, 10, kBackground);   // bright green
    init_pair(kWavesZPair, 3, kBackground);           // dark yellow
    init_pair(kWavesXPair, 9, kBackground);           // bright red
    init_pair(kWavesInlineValuePair, 7, kBackground); // light grey
    use_default_colors();
  }
}

void SetColor(WINDOW *w, int n) { wcolor_set(w, n, nullptr); }

} // namespace sv
