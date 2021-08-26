#pragma once

#include "panel.h"
#include "tree_data.h"

namespace sv {

class TreePanel : public Panel {
 public:
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const final { return data_.ListSize(); }
  bool Search(bool search_down) final;

 protected:
  TreeData data_;
  // Show or hide the +/- characters in front of expandable items.
  bool show_expanders_ = true;
  // Show the type before or after the item name.
  bool prepend_type_ = false;
  // Leave some area for the header.
  int header_lines_ = 0;

 private:
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
};

} // namespace sv
