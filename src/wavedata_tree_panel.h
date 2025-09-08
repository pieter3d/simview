#pragma once

#include "tree_panel.h"
#include "wavedata_tree_item.h"
#include <memory>

namespace sv {

// Holds and presents the wave data from the workspace in a browseable tree. The
// actual signals at each hierarchy level are not shown here, those are
// displayed using the WaveSignalsPanel.
class WaveDataTreePanel : public TreePanel {
 public:
  WaveDataTreePanel();
  void UIChar(int ch) final;
  std::vector<Tooltip> Tooltips() const final;
  bool Searchable() const final { return true; }
  std::optional<const WaveData::SignalScope *> ScopeForSignals();
  void HandleReloadedWaves() final;

 private:
  void BuildInitialTree();
  const WaveData::SignalScope *scope_for_signals_ = nullptr;
  std::vector<std::unique_ptr<WaveDataTreeItem>> roots_;
};

} // namespace sv
