#pragma once

#include "absl/container/flat_hash_map.h"
#include <string_view>
#include <vector>

namespace sv {

// This tokenizer processes a sequence of lines that are meant to be lines of a
// SystemVerilog source file, in order with none skipped. It will identify
// comment ranges, identifiers and keywords, knowing to skip over string
// literals and compiler directives / macros.
class SimpleTokenizer {
 public:
  void ProcessLine(std::string_view s);
  // Returns a list of ranges in the line that are part of comments.
  std::vector<std::pair<int, int>> &Comments(int line) { return comments_[line]; }
  // Returns a list of positions and strings that are identifiers in the line.
  std::vector<std::pair<int, std::string_view>> &Identifiers(int line) {
    return identifiers_[line];
  }
  // Returns a list of ranges that are keywords in the line.
  std::vector<std::pair<int, int>> &Keywords(int line) { return keywords_[line]; }

 private:
  int line_num_ = 0;
  bool in_block_comment_ = false;
  bool in_string_literal_ = false;
  bool last_token_was_dot_ = false;
  absl::flat_hash_map<int, std::vector<std::pair<int, int>>> comments_;
  absl::flat_hash_map<int, std::vector<std::pair<int, int>>> keywords_;
  absl::flat_hash_map<int, std::vector<std::pair<int, std::string_view>>> identifiers_;
};

} // namespace sv
