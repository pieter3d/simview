#pragma once

#include "radix.h"
#include "tree_item.h"
#include "wave_data.h"
#include <string>

namespace sv {

class WaveTreeItem : public TreeItem {
 public:
  WaveTreeItem() : blank_(true){};
  WaveTreeItem(std::string &s) : group_name_(s){};
  WaveTreeItem(const WaveData::Signal *s) : signal_(s) {}
  const std::string &Name() const final;
  const std::string &Type() const final;
  bool AltType() const final { return false; }
  bool Expandable() const final { return !children_.empty(); };
  int NumChildren() const final { return children_.size(); };
  TreeItem *Child(int idx) final { return &children_[idx]; }
  // Accessors
  const WaveData::Signal *Signal() const { return signal_; }
  const std::string &GroupName() const { return group_name_; }
  void SetGroupName(const std::string &s) { group_name_ = s; }
  int AnalogSize() const { return analog_size_; }
  void SetAnalogSize(int a) { analog_size_ = a; }
  bool Blank() const { return blank_; }
  // Mutators
  void AddChild(const WaveTreeItem *wti, int idx);
  void RemoveChild(int idx);
  void MoveChildUp(int idx);
  void MoveChildDown(int idx);
  std::string GetValue() const;
  void CycleRadix();

 private:
  Radix radix_ = Radix::kHex;
  const WaveData::Signal *signal_ = nullptr;
  const WaveTreeItem *parent_ = nullptr; // for grouped items.
  int analog_size_ = 1;
  const bool blank_ = false;
  // When not empty, the item is treated as a group container, regardless of any
  // signals.
  std::string group_name_;
  std::vector<WaveTreeItem> children_;
};

} // namespace sv
