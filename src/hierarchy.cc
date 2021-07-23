#include "hierarchy.h"

namespace sv {

void Hierarchy::Draw() const {
  werase(w_);
  // test
  mvwprintw(w_, 0, 0, "Boop!");
}

void Hierarchy::UIChar(int ch) {


}

} // namespace sv
