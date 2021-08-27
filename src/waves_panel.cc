#include "waves_panel.h"
#include "workspace.h"

namespace sv {

void WavesPanel::Draw() {
  werase(w_);
  // test
  mvwprintw(w_, 0, 0, "waves coming soon");
}

void WavesPanel::UIChar(int ch) {}

std::string WavesPanel::Tooltip() const {
  std::string tt;
  if (Workspace::Get().Design() != nullptr) tt += "s:source  ";
  tt += "F:zoom full  ";
  tt += "zZ:zoom  ";
  tt += "nN:names  ";
  tt += "vV:values  ";
  tt += "h:hierarchy  ";
  tt += "e:edge  ";
  tt += "m:marker  ";
  tt += "i:insert  ";
  tt += "b:blank  ";
  tt += "c:color  ";
  tt += "r:radix  ";
  tt += "aA:analog  ";
  return tt;
}
} // namespace sv
