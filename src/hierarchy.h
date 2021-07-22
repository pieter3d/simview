#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "panel.h"

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(WINDOW *w) : Panel(w) {}
  void Draw() const override;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
