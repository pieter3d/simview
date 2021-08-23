#pragma once

#include <string>

namespace sv {

// Single item in a tree structure.
class TreeItem {
 public:
  virtual std::string Name() const = 0;
  virtual std::string Type() const = 0;
  virtual bool AltType() const = 0;
  virtual bool Expandable() const = 0;
  virtual int NumChildren() const = 0;
  virtual TreeItem *Child(int idx) const = 0;

  // State update.
  void SetDepth(int d) { depth_ = d; }
  void SetExpanded(bool e) { expanded_ = e; }
  void SetMoreIdx(int m) { more_idx_ = m; }
  // Current state.
  int Depth() const { return depth_; }
  bool Expanded() const { return expanded_; }
  int MoreIdx() const { return more_idx_; }

 private:
  int depth_ = 0;
  bool expanded_ = false;
  // When non-zero, indicates there are more subs, but they were cut off at this
  // index. Helps keep a tree from being flooded when there are a ton of
  // children.
  int more_idx_ = 0;
};

} // namespace sv
