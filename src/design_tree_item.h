#pragma once

#include "tree_item.h"
#include <uhdm/headers/uhdm_types.h>
#include <vector>

namespace sv {

// Tree item that holds an entry in the design hierarchy.
class DesignTreeItem : public TreeItem {
 public:
  explicit DesignTreeItem(UHDM::any *item);
  const std::string &Name() const final { return name_; }
  const std::string &Type() const final { return type_; }
  bool AltType() const final;
  bool Expandable() const final;
  int NumChildren() const final;
  TreeItem *Child(int idx) final;
  bool MatchColor() const final;
  bool ErrType() const final;

  const UHDM::any *DesignItem() const { return item_; }

 private:
  // Not owned, this object must not outlive the item owner.
  const UHDM::any *item_;
  // Computed strings.
  std::string name_;
  std::string type_;
  // Lazy build for this. Has to be marked mutable since it gets called from
  // const methods.
  void BuildChildren() const;
  mutable bool children_built_ = false;
  mutable std::vector<DesignTreeItem> children_;
};

} // namespace sv
