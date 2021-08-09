#ifndef _SRC_COLOR_H_
#define _SRC_COLOR_H_

#include <ncurses.h>

namespace sv {

constexpr int kBorderPair = 1;
constexpr int kFocusBorderPair = 2;
constexpr int kOverflowTextPair = 3;
constexpr int kSearchPair = 4;
constexpr int kHierExpandPair = 5;
constexpr int kHierInstancePair = 6;
constexpr int kHierModulePair = 7;
constexpr int kHierShowMorePair = 8;
constexpr int kHierOtherPair = 9;
constexpr int kTooltipPair = 10;
constexpr int kTooltipKeyPair = 11;
constexpr int kSourceLineNrPair = 12;
constexpr int kSourceCurrentLineNrPair = 13;
constexpr int kSourceHeaderPair = 14;
constexpr int kSourceTextPair = 15;
constexpr int kSourceIdentifierPair = 16;
constexpr int kSourceKeywordPair = 17;
constexpr int kSourceCommentPair = 18;
constexpr int kSourceInactivePair = 19;
constexpr int kSourceInstancePair = 20;
constexpr int kSourceParamPair = 21;
constexpr int kSourceValuePair = 22;

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
#endif // _SRC_COLOR_H_
