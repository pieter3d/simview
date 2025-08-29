#include "source_buffer.h"

namespace sv {
namespace {

constexpr int kTabSize = 4;

} // namespace

void SourceBuffer::ProcessBuffer(std::string_view buf) {
  // Clear to start
  lines_.clear();
  tab_expansion_info_.clear();
  // Helper to process the most recently added line.
  auto AddTabInfo = [&] {
    std::string_view s = lines_.back();
    const int line_idx = lines_.size() - 1;
    int extra = 0;
    int pos = 0;
    bool any_tabs = false;
    int tab_pos;
    while ((tab_pos = s.find('\t', pos)) != std::string_view::npos) {
      pos = tab_pos + 1;
      const int display_pos = tab_pos + extra;
      // Calculate the number of spaces caused by this tab character, not counting the tab itself.
      // Accumulate since the number of extra spaces up to this location include all previous tabs.
      extra += kTabSize - display_pos % kTabSize - 1;
      tab_expansion_info_[line_idx].extra_spaces[pos] = extra;
      any_tabs = true;
    }
    if (any_tabs) tab_expansion_info_[line_idx].line_length = s.size() + extra;
  };
  size_t pos = 0;
  while (pos < buf.length()) {
    size_t newline_pos = buf.find_first_of("\r\n", pos);
    const bool is_cr = newline_pos != std::string_view::npos && buf[newline_pos] == '\r';
    // If a newline is found, extract the substring up to it.
    if (newline_pos != std::string_view::npos) {
      lines_.push_back(buf.substr(pos, newline_pos - pos));
      AddTabInfo();
      // Advance past the newline character for the next search.
      pos = newline_pos + 1;
      // Check for the Windows-style CRLF sequence (\r\n).
      if (pos < buf.length() && is_cr && buf[pos] == '\n') pos++;
    } else {
      // No more newlines found, so add the remaining part of the string and finish.
      int remainder = buf.length() - pos;
      // Skip the terminating zero.
      if (buf[buf.length() - 1] == '\0') remainder--;
      // Don't add a last blank line.
      if (remainder > 0) {
        lines_.push_back(buf.substr(pos, remainder));
        AddTabInfo();
      }
      break;
    }
  }
}

int SourceBuffer::LineLength(int n) const {
  const auto it = tab_expansion_info_.find(n);
  if (it == tab_expansion_info_.end()) return lines_[n].size();
  return it->second.line_length;
}

int SourceBuffer::DisplayCol(int row, int col) const {
  const auto line_map_it = tab_expansion_info_.find(row);
  if (line_map_it == tab_expansion_info_.end()) return col;
  const absl::btree_map<int, int> &map = line_map_it->second.extra_spaces;
  auto it = map.upper_bound(col);
  if (it == map.begin()) return col;
  it--;
  return col + it->second;
}

} // namespace sv
