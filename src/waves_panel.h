#pragma once

#include "panel.h"

namespace sv {

class WavesPanel : public Panel {
 public:
  WavesPanel(int h, int w, int row, int col) : Panel(h, w, row, col) {}
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return 0; }
  std::string Tooltip() const override { return ""; }

 private:
};

} // namespace sv
