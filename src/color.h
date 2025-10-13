#pragma once

#include <curses.h>

namespace sv {

// All colors in the UI are defined with NCurses pairs, which sets both
// foreground and backgound.
// TODO: Support custom colors with a settings file.
constexpr int kBorderPair = 1;
constexpr int kFocusBorderPair = 2;
constexpr int kPanelErrorPair = 3;
constexpr int kOverflowTextPair = 4;
constexpr int kTextInputPair = 5;
constexpr int kTextInputRejectPair = 6;
constexpr int kHierExpandPair = 7;
constexpr int kHierNamePair = 8;
constexpr int kHierTypePair = 9;
constexpr int kHierErrPair = 10;
constexpr int kHierShowMorePair = 11;
constexpr int kHierOtherPair = 12;
constexpr int kHierMatchedNamePair = 13;
constexpr int kTooltipPair = 14;
constexpr int kTooltipKeyPair = 15;
constexpr int kSourceLineNrPair = 16;
constexpr int kSourceCurrentLineNrPair = 17;
constexpr int kSourceHeaderPair = 18;
constexpr int kSourceTextPair = 19;
constexpr int kSourceIdentifierPair = 20;
constexpr int kSourceKeywordPair = 21;
constexpr int kSourceCommentPair = 22;
constexpr int kSourceInactivePair = 23;
constexpr int kSourceInstancePair = 24;
constexpr int kSourceParamPair = 25;
constexpr int kSourceValuePair = 26;
constexpr int kSignalToggleOnPair = 27;
constexpr int kSignalToggleOffPair = 28;
constexpr int kSignalFilterPair = 29;
constexpr int kWavesTimeTickPair = 30;
constexpr int kWavesTimeValuePair = 31;
constexpr int kWavesCursorPair = 32;
constexpr int kWavesMarkerPair = 33;
constexpr int kWavesDeltaPair = 34;
constexpr int kWavesSignalNamePair = 35;
constexpr int kWavesSignalValuePair = 36;
constexpr int kWavesGroupPair = 37;
// Increment by two here. These have a highlighted background variant.
constexpr int kWavesWaveformPair = 38;
constexpr int kWavesZPair = 40;
constexpr int kWavesXPair = 42;
constexpr int kWavesInlineValuePair = 44;
constexpr int kWavesCustomPair = 46;
// 10 custom colors, each with a background highlight variant. So need
// a step of 20.
constexpr int kReservedColor = 66;

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
