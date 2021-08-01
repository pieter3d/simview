#ifndef _SRC_WAVES_H_
#define _SRC_WAVES_H_

#include "panel.h"

namespace sv {

class Waves : public Panel {
 public:
  Waves(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  std::string Tooltip() const override { return ""; }

 private:
};

} // namespace sv
#endif // _SRC_WAVES_H_
