#pragma once

#include "tree_item.h"
#include "wave_data.h"

namespace sv {

class WaveTreeItem : public TreeItem {
 public:
  explicit WaveTreeItem(const WaveData::HierarchyLevel &hl);
  virtual const std::string &Name() const override;
  virtual const std::string &Type() const override;
  virtual bool AltType() const override;
  virtual bool Expandable() const override;
  virtual int NumChildren() const override;
  virtual TreeItem *Child(int idx) override;

 private:
  const WaveData::HierarchyLevel &hierarchy_level_;
  std::vector<WaveTreeItem> children_;
};

} // namespace sv
