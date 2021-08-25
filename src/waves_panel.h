#pragma once

#include "panel.h"

namespace sv {

class WavesPanel : public Panel {
 public:
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return 0; }
  std::string Tooltip() const override { return ""; }

 private:
};

} // namespace sv
