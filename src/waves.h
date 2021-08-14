#ifndef _SRC_WAVES_H_
#define _SRC_WAVES_H_

#include "panel.h"

namespace sv {

class Waves : public Panel {
 public:
  Waves(int h, int w, int row, int col) : Panel(h, w, row, col) {}
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return 0; }
  std::string Tooltip() const override { return ""; }

 private:
};

} // namespace sv
#endif // _SRC_WAVES_H_
