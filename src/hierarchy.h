#ifndef _SRC_HIERARCHY_H_
#define _SRC_HIERARCHY_H_

#include "panel.h"
#include <uhdm/headers/uhdm_types.h>
#include <unordered_map>

namespace sv {

class Hierarchy : public Panel {
 public:
  Hierarchy(int h, int w, int row, int col);
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return entries_.size(); }
  std::string Tooltip() const override;
  std::optional<std::pair<const UHDM::any *, bool>> ItemForSource();
  // Opens the hierarchy to the selected item.
  void SetItem(const UHDM::any *item);
  bool Search(bool search_down) override;

 private:
  struct EntryInfo {
    int depth = 0;
    bool expanded = false;
    bool expandable = false;
    bool is_generate = false;
    int more_idx = 0;
    const UHDM::any *parent = nullptr;
  };
  void ToggleExpand();

  std::vector<const UHDM::any *> entries_;
  std::unordered_map<const UHDM::any *, EntryInfo> entry_info_;
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
  bool load_instance_ = false;
  bool load_definition_ = false;
};

} // namespace sv
#endif // _SRC_HIERARCHY_H_
