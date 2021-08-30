#include "wave_tree_item.h"

namespace sv {

const std::string &WaveTreeItem::Name() const {
  if (children_.empty()) return group_name_;
  return signal_->name;
}
// Not used in this TreePanel implementation.
const std::string &WaveTreeItem::Type() const {
  static std::string empty_string;
  return empty_string;
}
void WaveTreeItem::AddChild(const WaveTreeItem *wti, int idx) {}
void WaveTreeItem::MoveChildUp(int idx) {}
void WaveTreeItem::MoveChildDown(int idx) {}
std::string WaveTreeItem::GetValue() const { return ""; }

void WaveTreeItem::CycleRadix() {
  switch (radix_) {
  case Radix::kHex: radix_ = Radix::kBinary; break;
  case Radix::kBinary:
    // Don't bother with decimal or float on huge values.
    radix_ = signal_->width > 64 ? Radix::kHex : Radix::kSignedDecimal;
    break;
  case Radix::kSignedDecimal: radix_ = Radix::kUnsignedDecimal; break;
  case Radix::kUnsignedDecimal:
    // Float only makes sense for fp32/fp64 formats.
    radix_ = signal_->width == 32 || signal_->width == 64 ? Radix::kFloat
                                                          : Radix::kHex;
    break;
  case Radix::kFloat: radix_ = Radix::kHex; break;
  }
}

} // namespace sv
