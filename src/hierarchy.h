#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "panel.h"
#include <Design/Design.h>
#include <Design/ModuleInstance.h>
#include <unordered_set>

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(WINDOW *w);
  void SetDesign(SURELOG::Design *d);
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  SURELOG::ModuleInstance *InstanceForSource();

 private:
  struct InstanceLine {
    SURELOG::ModuleInstance *instance;
    int depth;
    bool expanded = false;
    bool expandable = false;
    int max_subs = 0;
  };
  void ToggleExpand();
  void ExpandAt(int idx);

  SURELOG::Design *design_;
  std::vector<InstanceLine> instances_;
  std::unordered_set<SURELOG::ModuleInstance *> expanded_instances_;
  int ui_line_index_ = 0;
  int ui_row_scroll_ = 0;
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
  bool load_source_ = false;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
