#ifndef _SRC_SIMPLE_TOKENIZER_H_
#define _SRC_SIMPLE_TOKENIZER_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace sv {

// This tokenizer processes a sequence of lines that are meant to be lines of a
// SystemVerilog source file, in order with none skipped. It will identify
// comment ranges, identifiers and keywords, knowing to skip over string
// literals and compiler directives / macros.
//
class SimpleTokenizer {
 public:
  void ProcessLine(const std::string &s);
  // Returns a list of ranges in the line that are part of comments.
  std::vector<std::pair<int, int>> &CommentRanges(int line) {
    return comment_ranges_[line];
  }
  // Returns a list positions and strings that are idenfiers in the line.
  std::vector<std::pair<int, std::string>> &Identifiers(int line) {
    return identifiers_[line];
  }
  // Returns a list positions and strings that are keywords in the line.
  std::vector<std::pair<int, std::string>> &Keywords(int line) {
    return keywords_[line];
  }

 private:
  int line_num_ = 0;
  bool in_block_comment_ = false;
  bool in_string_literal_ = false;
  std::unordered_map<int, std::vector<std::pair<int, int>>> comment_ranges_;
  std::unordered_map<int, std::vector<std::pair<int, std::string>>>
      identifiers_;
  std::unordered_map<int, std::vector<std::pair<int, std::string>>> keywords_;
};

} // namespace sv
#endif // _SRC_SIMPLE_TOKENIZER_H_