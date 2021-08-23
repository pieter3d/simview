#pragma once

#include "design_tree_item.h"
#include "tree_panel.h"
#include <memory>
#include <uhdm/headers/uhdm_types.h>

namespace sv {

class DesignTreePanel : public TreePanel {
 public:
  DesignTreePanel(int h, int w, int row, int col);
  void UIChar(int ch) override;
  std::string Tooltip() const override;
  // Get the item that should be shown in the source panel, if any.
  // The bool indicates to load the definition instead of the instance location.
  std::optional<std::pair<const UHDM::any *, bool>> ItemForSource();
  // Opens the tree to the selected item.
  void SetItem(const UHDM::any *item);
  bool Searchable() override { return true; }

 private:
  std::vector<std::unique_ptr<DesignTreeItem>> roots_;
  bool load_instance_ = false;
  bool load_definition_ = false;
};

} // namespace sv
