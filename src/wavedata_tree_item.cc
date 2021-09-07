#include "wavedata_tree_item.h"
#include "workspace.h"

namespace sv {

WaveDataTreeItem::WaveDataTreeItem(const WaveData::SignalScope &signal_scope)
    : signal_scope_(signal_scope) {
  for (const auto &c : signal_scope_.children) {
    children_.push_back(WaveDataTreeItem(c));
  }
}
const std::string &WaveDataTreeItem::Name() const { return signal_scope_.name; }

const std::string &WaveDataTreeItem::Type() const {
  static std::string empty = "";
  return empty;
}

bool WaveDataTreeItem::AltType() const { return false; }

bool WaveDataTreeItem::Expandable() const { return !children_.empty(); }

int WaveDataTreeItem::NumChildren() const { return children_.size(); }

TreeItem *WaveDataTreeItem::Child(int idx) { return &children_[idx]; }

bool WaveDataTreeItem::MatchColor() const {
  return &signal_scope_ == Workspace::Get().MatchedSignalScope();
}
} // namespace sv
