#include "wavedata_tree_item.h"

namespace sv {

WaveDataTreeItem::WaveDataTreeItem(const WaveData::SignalScope &hl)
    : hierarchy_level_(hl) {
  for (const auto &c : hierarchy_level_.children) {
    children_.push_back(WaveDataTreeItem(c));
  }
}
const std::string &WaveDataTreeItem::Name() const {
  return hierarchy_level_.name;
}

const std::string &WaveDataTreeItem::Type() const {
  static std::string empty = "";
  return empty;
}

bool WaveDataTreeItem::AltType() const { return false; }

bool WaveDataTreeItem::Expandable() const { return !children_.empty(); }

int WaveDataTreeItem::NumChildren() const { return children_.size(); }

TreeItem *WaveDataTreeItem::Child(int idx) { return &children_[idx]; }

} // namespace sv
