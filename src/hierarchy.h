#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "panel.h"
#include <surelog/surelog.h>

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(WINDOW *w) : Panel(w) {}
  void SetDesign(SURELOG::Design *d) { design_ = d; }
  void Draw() const override;
  void UIChar(int ch) override;

 private:
  SURELOG::Design *design_;
  int tmp;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
