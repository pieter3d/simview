#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "panel.h"
#include <Design/Design.h>
#include <Design/ModuleInstance.h>
#include <unordered_map>

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(WINDOW *w);
  void SetDesign(SURELOG::Design *d);
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  std::string Tooltip() const override;
  SURELOG::ModuleInstance *InstanceForSource();

 private:
  struct EntryInfo {
    int depth = 0;
    bool expanded = false;
    bool expandable = false;
    int more_idx = 0;
  };
  void ToggleExpand();

  SURELOG::Design *design_;
  std::vector<SURELOG::ModuleInstance *> entries_;
  std::unordered_map<SURELOG::ModuleInstance *, EntryInfo> entry_info_;
  int ui_line_index_ = 0;
  int ui_row_scroll_ = 0;
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
  bool load_source_ = false;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
