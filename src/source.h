#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "Design/ModuleInstance.h"
#include "panel.h"

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  std::string Tooltip() const override { return ""; }
  void SetInstance(SURELOG::ModuleInstance *inst) { inst_ = inst; }

 private:
  SURELOG::ModuleInstance *inst_ = nullptr;
};

} // namespace sv
#endif // _SRC_SOURCE_H_
