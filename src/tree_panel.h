#ifndef _SRC_TREE_PANEL_H_
#define _SRC_TREE_PANEL_H_

#include "panel.h"
#include "tree_data.h"

namespace sv {

class TreePanel : public Panel {
 public:
  TreePanel(int h, int w, int row, int col) : Panel(h, w, row, col) {}
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return data_.ListSize(); }
  bool Search(bool search_down) override;

 protected:
  TreeData data_;

 private:
  int ui_col_scroll_ = 0;
  int ui_max_col_scroll_ = 0;
};

} // namespace sv
#endif // _SRC_TREE_PANEL_H_
