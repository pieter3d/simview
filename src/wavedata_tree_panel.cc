#include "wavedata_tree_panel.h"
#include "workspace.h"
#include <optional>

namespace sv {

WaveDataTreePanel::WaveDataTreePanel() {
  if (Workspace::Get().Waves() == nullptr) return;
  root_ = std::make_unique<WaveDataTreeItem>(Workspace::Get().Waves()->Root());
  data_.AddRoot(root_.get());
  // Expand a little bit if it's easy.
  data_.ToggleExpand(0);
  if (data_.ListSize() == 2) {
    data_.ToggleExpand(1);
  }
}

void WaveDataTreePanel::UIChar(int ch) {
  int initial_line = line_idx_;
  switch (ch) {
  default: TreePanel::UIChar(ch);
  }
  // If the selection moved, update the signals panel
  if (line_idx_ != initial_line) {
    scope_for_signals_ =
        dynamic_cast<WaveDataTreeItem *>(data_[line_idx_])->SignalScope();
  }
}

std::string WaveDataTreePanel::Tooltip() const { return ""; }

std::optional<const WaveData::SignalScope *>
WaveDataTreePanel::ScopeForSignals() {
  if (scope_for_signals_ == nullptr) return std::nullopt;
  auto ptr = scope_for_signals_;
  scope_for_signals_ = nullptr;
  return ptr;
}

} // namespace sv
