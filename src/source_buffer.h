#pragma once

#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include <string_view>

namespace sv {

class SourceBuffer {
 public:
  void ProcessBuffer(std::string_view buf);
  bool Empty() const { return lines_.empty(); }
  int NumLines() const { return lines_.size(); }
  std::string_view operator[](int n) const { return lines_[n]; }
  // Returns the length of the given source line, accounting for tab expansion.
  int LineLength(int n) const;
  // Compute the column location of the given location, accounting for tab expansion.
  int DisplayCol(int row, int col) const;

  // Allow for range based iteration
  std::vector<std::string_view>::iterator begin() { return lines_.begin(); }
  std::vector<std::string_view>::iterator end() { return lines_.end(); }

 private:
  std::vector<std::string_view> lines_;
  struct TabInfo {
    int line_length;
    absl::btree_map<int, int> extra_spaces;
  };
  absl::flat_hash_map<int, TabInfo> tab_expansion_info_;
};

} // namespace sv
