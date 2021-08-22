#ifndef _SRC_DESIGN_TREE_ITEM_H_
#define _SRC_DESIGN_TREE_ITEM_H_

#include "tree_item.h"
#include <uhdm/headers/uhdm_types.h>
#include <vector>

namespace sv {

class DesignTreeItem : public TreeItem {
 public:
  explicit DesignTreeItem(UHDM::any *item) : item_(item) {}
  virtual std::string Name() const override;
  virtual std::string Type() const override;
  virtual bool AltType() const override;
  virtual bool Expandable() const override;
  virtual int NumChildren() const override;
  virtual TreeItem *Child(int idx) const override;

  const UHDM::any *DesignItem() const { return item_; }

 private:
  // Not owned, this object must not outlive the item owner.
  const UHDM::any *item_;
  // Lazy build for this. Has to be marked const since it gets called from const
  // methods.
  void BuildChildren() const;
  mutable bool children_built_ = false;
  mutable std::vector<DesignTreeItem> children_;
};

} // namespace sv
#endif // _SRC_DESIGN_TREE_ITEM_H_
