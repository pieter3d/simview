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
  void UIChar(int ch) override;
  std::string Tooltip() const override;
  bool Searchable() const override { return true; }
  std::optional<const WaveData::SignalScope *> ScopeForSignals();

 private:
  const WaveData::SignalScope *scope_for_signals_ = nullptr;
  std::unique_ptr<WaveDataTreeItem> root_;
};

} // namespace sv
