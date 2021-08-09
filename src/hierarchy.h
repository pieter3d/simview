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
  Hierarchy(int h, int w, int row, int col);
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return entries_.size(); }
  std::string Tooltip() const override;
  bool Search(const std::string &s, bool preview) { return false; }
  std::optional<std::pair<const UHDM::BaseClass *, bool>> ItemForSource();
  // Opens the hierarchy to the selected item.
  void SetItem(const UHDM::BaseClass *item);

 private:
  struct EntryInfo {
    int depth = 0;
    bool expanded = false;
    bool expandable = false;
    bool is_generate = false;
    int more_idx = 0;
    const UHDM::BaseClass *parent = nullptr;
  };
  void ToggleExpand();

  std::vector<const UHDM::BaseClass *> entries_;
  std::unordered_map<const UHDM::BaseClass *, EntryInfo> entry_info_;
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
  bool load_instance_ = false;
  bool load_definition_ = false;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
