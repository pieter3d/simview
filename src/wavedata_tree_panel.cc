#include "wavedata_tree_panel.h"
#include "workspace.h"
#include <optional>

namespace sv {

WaveDataTreePanel::WaveDataTreePanel() {
  if (Workspace::Get().Waves() == nullptr) return;
  for (const auto &scope : Workspace::Get().Waves()->Roots()) {
    roots_.push_back(std::make_unique<WaveDataTreeItem>(scope));
    data_.AddRoot(roots_.back().get());
  }
  // Expand a little bit if it's easy.
  data_.ToggleExpand(0);
  if (data_.ListSize() == 2) {
    data_.ToggleExpand(1);
  }
}

void WaveDataTreePanel::UIChar(int ch) {
  int initial_line = line_idx_;
  switch (ch) {
  case 'S':
    Workspace::Get().SetMatchedSignalScope(
        dynamic_cast<const WaveDataTreeItem *>(data_[line_idx_])
            ->SignalScope());
    break;
  default: TreePanel::UIChar(ch);
  }
  // If the selection moved, update the signals panel
  if (line_idx_ != initial_line) {
    scope_for_signals_ =
        dynamic_cast<WaveDataTreeItem *>(data_[line_idx_])->SignalScope();
  }
}

std::vector<Tooltip> WaveDataTreePanel::Tooltips() const {
  return std::vector<Tooltip>{{"S", "set scope for source"}};
}

std::optional<const WaveData::SignalScope *>
WaveDataTreePanel::ScopeForSignals() {
  if (scope_for_signals_ == nullptr) return std::nullopt;
  auto ptr = scope_for_signals_;
  scope_for_signals_ = nullptr;
  return ptr;
}

} // namespace sv
