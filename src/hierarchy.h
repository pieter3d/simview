#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "panel.h"
#include <Design/Design.h>
#include <Design/ModuleInstance.h>

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(WINDOW *w);
  void SetDesign(SURELOG::Design *d);
  void Draw() const override;
  void UIChar(int ch) override;

 private:
  struct InstanceLine {
    SURELOG::ModuleInstance *instance;
    int depth;
    int index_in_parent;
    bool expanded = false;
  };
  void ToggleExpand(SURELOG::ModuleInstance *inst);

  SURELOG::Design *design_;
  SURELOG::ModuleInstance *current_instance_;
  std::vector<InstanceLine> instances_;
  int ui_line_index_ = 0;
  int instance_index_ = 0;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
