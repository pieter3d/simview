#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "panel.h"

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;

 private:
};

} // namespace sv
#endif // _SRC_SOURCE_H_
