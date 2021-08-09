#ifndef _SRC_COLOR_H_
#define _SRC_COLOR_H_

#include <ncurses.h>

namespace sv {

constexpr int kBorderPair = 1;
constexpr int kFocusBorderPair = 2;
constexpr int kOverflowTextPair = 3;
constexpr int kSearchPair = 4;
constexpr int kSearchNotFoundPair = 5;
constexpr int kHierExpandPair = 6;
constexpr int kHierInstancePair = 7;
constexpr int kHierModulePair = 8;
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

void SetupColors();
void SetColor(WINDOW *w, int n);

} // namespace sv
#endif // _SRC_COLOR_H_
