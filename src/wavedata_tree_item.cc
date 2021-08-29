#include "wavedata_tree_item.h"

namespace sv {

WaveTreeItem::WaveTreeItem(const WaveData::SignalScope &hl)
    : hierarchy_level_(hl) {
  for (const auto &c : hierarchy_level_.children) {
    children_.push_back(WaveTreeItem(c));
  }
}
const std::string &WaveTreeItem::Name() const { return hierarchy_level_.name; }

const std::string &WaveTreeItem::Type() const {
  static std::string empty = "";
  return empty;
}

bool WaveTreeItem::AltType() const { return false; }

bool WaveTreeItem::Expandable() const { return !children_.empty(); }

int WaveTreeItem::NumChildren() const { return children_.size(); }

TreeItem *WaveTreeItem::Child(int idx) { return &children_[idx]; }

} // namespace sv
