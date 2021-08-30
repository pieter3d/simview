#pragma once

#include "tree_item.h"
#include "wave_data.h"

namespace sv {

class SignalTreeItem : public TreeItem {
 public:
  SignalTreeItem(const WaveData::Signal &s) : signal_(s) {}
  const std::string &Name() const final;
  const std::string &Type() const final;
  bool AltType() const final;
  const WaveData::Signal *Signal() const { return &signal_; }
  // Flat data, no tree stuff.
  virtual bool Expandable() const final { return false; }
  virtual int NumChildren() const final { return 0; }
  virtual TreeItem *Child(int idx) { return nullptr; }

 private:
  const WaveData::Signal &signal_;
};

} // namespace sv
