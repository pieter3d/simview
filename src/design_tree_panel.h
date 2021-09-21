#pragma once

#include "design_tree_item.h"
#include "tree_panel.h"
#include <memory>
#include <uhdm/uhdm_types.h>

namespace sv {

// A Panel that shows the design hierarchy.
class DesignTreePanel : public TreePanel {
 public:
  DesignTreePanel();
  void UIChar(int ch) final;
  std::string Tooltip() const final;
  // Get the item that should be shown in the source panel, if any.
  // The bool indicates to load the definition instead of the instance location.
  std::optional<std::pair<const UHDM::any *, bool>> ItemForSource();
  // Opens the tree to the selected item.
  void SetItem(const UHDM::any *item);
  bool Searchable() const final { return true; }

 private:
  std::vector<std::unique_ptr<DesignTreeItem>> roots_;
  bool load_instance_ = false;
  bool load_definition_ = false;
};

} // namespace sv
