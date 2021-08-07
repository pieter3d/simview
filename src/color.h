#ifndef _SRC_COLOR_H_
#define _SRC_COLOR_H_

#include <ncurses.h>

namespace sv {

constexpr int kBorderPair = 1;
constexpr int kFocusBorderPair = 2;
constexpr int kOverflowTextPair = 3;
constexpr int kHierExpandPair = 4;
constexpr int kHierInstancePair = 5;
constexpr int kHierModulePair = 6;
constexpr int kHierShowMorePair = 7;
constexpr int kHierOtherPair = 8;
constexpr int kTooltipPair = 9;
constexpr int kTooltipKeyPair = 10;
constexpr int kSourceLineNrPair = 11;
constexpr int kSourceCurrentLineNrPair = 12;
constexpr int kSourceHeaderPair = 13;
constexpr int kSourceTextPair = 14;
constexpr int kSourceIdentifierPair = 15;
constexpr int kSourceKeywordPair = 16;
constexpr int kSourceCommentPair = 17;
constexpr int kSourceInactivePair = 18;
constexpr int kSourceInstancePair = 19;
constexpr int kSourceParamPair = 20;
constexpr int kSourceValuePair = 21;

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
#endif // _SRC_COLOR_H_
