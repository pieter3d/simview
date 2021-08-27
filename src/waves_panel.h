#pragma once

#include "panel.h"

namespace sv {

class WavesPanel : public Panel {
 public:
  void Draw() final;
  void UIChar(int ch) final;
  int NumLines() const final { return 0; }
  std::string Tooltip() const final;

 private:
};

} // namespace sv
