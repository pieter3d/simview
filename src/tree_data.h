#pragma once

#include "tree_item.h"
#include <vector>

namespace sv {

// Flat view of a slice of a tree, depending on which items are expanded or not.
class TreeData {
 public:
  int ListSize() const { return list_.size(); }
  bool Empty() const { return list_.empty(); }
  void ToggleExpand(int idx);
  const TreeItem *operator[](int idx) const { return list_[idx]; }
  TreeItem *operator[](int idx) { return list_[idx]; }
  void AddRoot(TreeItem *r) { list_.push_back(r); }
  void Clear() { list_.clear(); }

 private:
  // TreeItem is an abstract class, so gotta use pointers.
  std::vector<TreeItem *> list_;
};

} // namespace sv
