#pragma once

#include "slang/ast/Symbol.h"
#include "tree_item.h"
#include <vector>

namespace sv {

// Tree item that holds an entry in the design hierarchy.
class DesignTreeItem : public TreeItem {
 public:
  explicit DesignTreeItem(const slang::ast::Symbol *item);
  DesignTreeItem(const slang::ast::Symbol *item, int idx);
  std::string_view Name() const final { return name_.empty() ? item_->name : name_; }
  std::string_view Type() const final { return type_; }
  bool AltType() const final;
  bool Expandable() const final;
  int NumChildren() const final;
  TreeItem *Child(int idx) final;
  bool MatchColor() const final;
  bool ErrType() const final;

  const slang::ast::Symbol *DesignItem() const { return item_; }

 private:
  // Not owned, this object must not outlive the item owner.
  const slang::ast::Symbol *item_;
  // Computed strings.
  std::string type_;
  std::string name_;
  // Lazy build for this. Has to be marked mutable since it gets called from
  // const methods.
  void BuildChildren() const;
  mutable bool children_built_ = false;
  mutable std::vector<DesignTreeItem> children_;
  int index_ = 0;
};

} // namespace sv
