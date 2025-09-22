#pragma once

#include "design_tree_item.h"
#include "slang/ast/Symbol.h"
#include "tree_panel.h"
#include <memory>

namespace sv {

// A Panel that shows the design hierarchy.
class DesignTreePanel : public TreePanel {
 public:
  DesignTreePanel();
  void Initialize();
  void UIChar(int ch) final;
  std::vector<Tooltip> Tooltips() const final;
  // Get the item that should be shown in the source panel, if any.
  std::optional<const slang::ast::Symbol *> ItemForSource();
  // Opens the tree to the selected item.
  void SetItem(const slang::ast::Symbol *item);
  bool Searchable() const final { return true; }
  void HandleReloadedDesign() final;

 private:
  std::vector<std::unique_ptr<DesignTreeItem>> roots_;
  bool load_instance_ = false;
  bool load_definition_ = false;
};

} // namespace sv
