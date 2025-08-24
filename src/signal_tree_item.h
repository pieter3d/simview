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
  explicit SignalTreeItem(const WaveData::Signal *s);
  std::string_view Name() const final { return name_; }
  std::string_view Type() const final;
  bool AltType() const final;
  const WaveData::Signal *Signal() const { return signal_; }
  // Flat data, no tree stuff.
  bool Expandable() const final { return false; }
  int NumChildren() const final { return 0; }
  TreeItem *Child(int /*idx*/) final { return nullptr; }

 private:
  const WaveData::Signal *signal_;
  std::string name_;
};

} // namespace sv
