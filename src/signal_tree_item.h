#pragma once

#include "tree_item.h"
#include "wave_data.h"

namespace sv {

class SignalTreeItem : public TreeItem {
 public:
  SignalTreeItem(const WaveData::Signal &s) : signal_(s) {}
  const std::string &Name() const override;
  const std::string &Type() const override;
  bool AltType() const override;
  // Flat data, no tree stuff.
  virtual bool Expandable() const override { return false; }
  virtual int NumChildren() const override { return 0; }
  virtual TreeItem *Child(int idx) { return nullptr; }

 private:
  const WaveData::Signal &signal_;
};

} // namespace sv
