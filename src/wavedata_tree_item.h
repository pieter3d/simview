#pragma once

#include "tree_item.h"
#include "wave_data.h"

namespace sv {

// Tree data for the WaveData, allowing browsing of the scopes in a WaveData
// object.
class WaveDataTreeItem : public TreeItem {
 public:
  explicit WaveDataTreeItem(const WaveData::SignalScope &signal_scope);
  std::string_view Name() const final;
  std::string_view Type() const final;
  bool AltType() const final;
  bool Expandable() const final;
  int NumChildren() const final;
  TreeItem *Child(int idx) final;
  bool MatchColor() const final;

  const WaveData::SignalScope *SignalScope() const { return &signal_scope_; }

 private:
  const WaveData::SignalScope &signal_scope_;
  std::vector<WaveDataTreeItem> children_;
};

} // namespace sv
