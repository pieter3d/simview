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
  void Draw() override;
  void UIChar(int ch) override;

 private:
  struct InstanceLine {
    SURELOG::ModuleInstance *instance;
    int depth;
    bool expanded = false;
    bool expandable = false;
    int more_idx = 0;
  };
  void ToggleExpand();

  SURELOG::Design *design_;
  SURELOG::ModuleInstance *current_instance_;
  std::vector<InstanceLine> instances_;
  int ui_line_index_ = 0;
  int ui_row_scroll_ = 0;
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
