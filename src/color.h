#pragma once

#include <ncurses.h>

namespace sv {

constexpr int kBorderPair = 1;
constexpr int kFocusBorderPair = 2;
constexpr int kOverflowTextPair = 3;
constexpr int kTextInputPair = 4;
constexpr int kTextInputRejectPair = 5;
constexpr int kHierExpandPair = 6;
constexpr int kHierNamePair = 7;
constexpr int kHierTypePair = 8;
constexpr int kHierShowMorePair = 9;
constexpr int kHierOtherPair = 10;
constexpr int kHierMatchedNamePair = 11;
constexpr int kTooltipPair = 12;
constexpr int kTooltipKeyPair = 13;
constexpr int kSourceLineNrPair = 14;
constexpr int kSourceCurrentLineNrPair = 15;
constexpr int kSourceHeaderPair = 16;
constexpr int kSourceTextPair = 17;
constexpr int kSourceIdentifierPair = 18;
constexpr int kSourceKeywordPair = 19;
constexpr int kSourceCommentPair = 20;
constexpr int kSourceInactivePair = 21;
constexpr int kSourceInstancePair = 22;
constexpr int kSourceParamPair = 23;
constexpr int kSourceValuePair = 24;
constexpr int kSignalToggleOnPair = 25;
constexpr int kSignalToggleOffPair = 26;
constexpr int kSignalFilterPair = 27;
constexpr int kWavesTimeTickPair = 28;
constexpr int kWavesTimeValuePair = 29;
constexpr int kWavesCursorPair = 30;
constexpr int kWavesMarkerPair = 31;
constexpr int kWavesDeltaPair = 32;
constexpr int kWavesSignalNamePair = 33;
constexpr int kWavesSignalValuePair = 34;
constexpr int kWavesGroupPair = 35;
// Increment by two here. These have a highlighted background variant.
constexpr int kWavesWaveformPair = 36;
constexpr int kWavesZPair = 38;
constexpr int kWavesXPair = 40;
constexpr int kWavesInlineValuePair = 42;

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
