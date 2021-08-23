#pragma once

#include "tree_item.h"
#include <uhdm/headers/uhdm_types.h>
#include <vector>

namespace sv {

class DesignTreeItem : public TreeItem {
 public:
  explicit DesignTreeItem(UHDM::any *item);
  virtual const std::string &Name() const override { return name_; }
  virtual const std::string &Type() const override { return type_; }
  virtual bool AltType() const override;
  virtual bool Expandable() const override;
  virtual int NumChildren() const override;
  virtual TreeItem *Child(int idx) override;

  const UHDM::any *DesignItem() const { return item_; }

 private:
  // Not owned, this object must not outlive the item owner.
  const UHDM::any *item_;
  // Computed strings.
  std::string name_;
  std::string type_;
  // Lazy build for this. Has to be marked const since it gets called from const
  // methods.
  void BuildChildren() const;
  mutable bool children_built_ = false;
  mutable std::vector<DesignTreeItem> children_;
};

} // namespace sv
