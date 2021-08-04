#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "BaseClass.h"
#include "panel.h"
#include <uhdm/headers/BaseClass.h>
#include <uhdm/headers/design.h>
#include <unordered_map>

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(WINDOW *w, UHDM::design *d);
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return entries_.size(); }
  bool TransferPending() override;
  std::string Tooltip() const override;
  std::pair<UHDM::BaseClass *, bool> ItemForSource();

 private:
  struct EntryInfo {
    int depth = 0;
    bool expanded = false;
    bool expandable = false;
    bool is_generate = false;
    int more_idx = 0;
    UHDM::BaseClass *parent = nullptr;
  };
  void ToggleExpand();

  UHDM::design *design_;
  std::vector<UHDM::BaseClass *> entries_;
  std::unordered_map<UHDM::BaseClass *, EntryInfo> entry_info_;
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
  bool load_instance_ = false;
  bool load_definition_ = false;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
