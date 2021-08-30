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
constexpr int kTooltipPair = 11;
constexpr int kTooltipKeyPair = 12;
constexpr int kSourceLineNrPair = 13;
constexpr int kSourceCurrentLineNrPair = 14;
constexpr int kSourceHeaderPair = 15;
constexpr int kSourceTextPair = 16;
constexpr int kSourceIdentifierPair = 17;
constexpr int kSourceKeywordPair = 18;
constexpr int kSourceCommentPair = 19;
constexpr int kSourceInactivePair = 20;
constexpr int kSourceInstancePair = 21;
constexpr int kSourceParamPair = 22;
constexpr int kSourceValuePair = 23;
constexpr int kSignalToggleOnPair = 24;
constexpr int kSignalToggleOffPair = 25;
constexpr int kSignalFilterPair = 26;
constexpr int kWavesTimeTickPair = 27;
constexpr int kWavesTimeValuePair = 28;
constexpr int kWavesCursorPair = 29;
constexpr int kWavesMarkerPair = 30;
constexpr int kWavesDeltaPair = 31;
constexpr int kWavesSignalNamePair = 32;
constexpr int kWavesSignalValuePair = 33;
constexpr int kWavesGroupPair = 34;
constexpr int kWavesInsertPair = 35;

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
