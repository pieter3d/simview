#pragma once

#include "tree_item.h"
#include "wave_data.h"

namespace sv {

// Holds a signal for the signal list that comes from a single scope. The
// TreeItem API is used to have a consistent list navigating UI eventhough this
// data isn't actually hierarchical (just a flat list of signals for the given
// scope).
class SignalTreeItem : public TreeItem {
 public:
  SignalTreeItem(const WaveData::Signal *s);
  const std::string &Name() const final { return name_; }
  const std::string &Type() const final;
  bool AltType() const final;
  const WaveData::Signal *Signal() const { return signal_; }
  // Flat data, no tree stuff.
  virtual bool Expandable() const final { return false; }
  virtual int NumChildren() const final { return 0; }
  virtual TreeItem *Child(int idx) { return nullptr; }

 private:
  const WaveData::Signal *signal_;
  std::string name_;
};

} // namespace sv
