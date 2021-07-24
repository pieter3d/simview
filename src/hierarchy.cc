#include "hierarchy.h"

namespace sv {

void Hierarchy::Draw() const {
  werase(w_);
  // test
  mvwprintw(w_, 0, 0, "0x%x", tmp);
}

void Hierarchy::UIChar(int ch) {
tmp = ch;

}

} // namespace sv
