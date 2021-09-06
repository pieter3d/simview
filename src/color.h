#pragma once

#include <ncurses.h>

namespace sv {

constexpr int kBorderPair = 1;
constexpr int kFocusBorderPair = 2;
constexpr int kPanelErrorPair = 3;
constexpr int kOverflowTextPair = 4;
constexpr int kTextInputPair = 5;
constexpr int kTextInputRejectPair = 6;
constexpr int kHierExpandPair = 7;
constexpr int kHierNamePair = 8;
constexpr int kHierTypePair = 9;
constexpr int kHierShowMorePair = 10;
constexpr int kHierOtherPair = 11;
constexpr int kHierMatchedNamePair = 12;
constexpr int kTooltipPair = 13;
constexpr int kTooltipKeyPair = 14;
constexpr int kSourceLineNrPair = 15;
constexpr int kSourceCurrentLineNrPair = 16;
constexpr int kSourceHeaderPair = 17;
constexpr int kSourceTextPair = 18;
constexpr int kSourceIdentifierPair = 19;
constexpr int kSourceKeywordPair = 20;
constexpr int kSourceCommentPair = 21;
constexpr int kSourceInactivePair = 22;
constexpr int kSourceInstancePair = 23;
constexpr int kSourceParamPair = 24;
constexpr int kSourceValuePair = 25;
constexpr int kSignalToggleOnPair = 26;
constexpr int kSignalToggleOffPair = 27;
constexpr int kSignalFilterPair = 28;
constexpr int kWavesTimeTickPair = 29;
constexpr int kWavesTimeValuePair = 30;
constexpr int kWavesCursorPair = 31;
constexpr int kWavesMarkerPair = 32;
constexpr int kWavesDeltaPair = 33;
constexpr int kWavesSignalNamePair = 34;
constexpr int kWavesSignalValuePair = 35;
constexpr int kWavesGroupPair = 36;
// Increment by two here. These have a highlighted background variant.
constexpr int kWavesWaveformPair = 37;
constexpr int kWavesZPair = 39;
constexpr int kWavesXPair = 41;
constexpr int kWavesInlineValuePair = 43;
constexpr int kWavesCustomPair = 45;
// 10 custom colors, each with a background highlight variant. So need
// a step of 20.
constexpr int kReservedColor = 65;

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
