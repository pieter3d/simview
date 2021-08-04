#include "source.h"
#include "color.h"
#include "utils.h"
#include <curses.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <uhdm/headers/gen_scope.h>
#include <uhdm/headers/gen_scope_array.h>
#include <uhdm/headers/module.h>
#include <uhdm/headers/variables.h>

namespace sv {
namespace {
// Remove newline characters from the start or end of the string.
void trim_string(std::string &s) {
  if (s.empty()) return;
  if (s.size() > 1 && s[s.size() - 2] == '\r' && s[s.size() - 1] == '\n') {
    s.erase(s.size() - 2, 1);
  }
  if (s[s.size() - 1] == '\n') s.erase(s.size() - 1, 1);
}

int num_decimal_digits(int n) {
  int ret = 0;
  do {
    ret++;
    n /= 10;
  } while (n != 0);
  return ret;
}

} // namespace

std::string Source::GetHeader(int max_w = 0) {
  std::string type;
  switch (item_->VpiType()) {
  case vpiModule: {
    auto m = reinterpret_cast<UHDM::module *>(item_);
    type = m->VpiFullName();
    break;
  }
  case vpiGenScopeArray: {
    auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item_);
    type = ga->VpiFullName();
    break;
  }
  default: type = "Unknown type: " + std::to_string(item_->VpiType()); break;
  }
  const std::string separator = " | ";
  auto s = current_file_ + separator + StripWorklib(type);
  // Attempt to strip as many leading directories from the front of the header
  // until it fits. If it still doesn't fit then oh well, it will get cut off
  // in the Draw funtion.
  if (max_w != 0 && s.size() > max_w) {
    const char sep = std::filesystem::path::preferred_separator;
    // No need to include the first character. It's either the leading
    // separator which is shorter than the replacement ellipsis, or it's the
    // leading ellipsis from previous iterations.
    int search_pos = 1;
    int pos = -1;
    do {
      int result = current_file_.find(sep, search_pos);
      if (result == std::string::npos) break;
      pos = result;
      search_pos = pos + 1;
      // Keep going while the new string length still doesn't fit.
      // Account for the ellipsis, 3 dots: ...
    } while (s.size() - pos + 3 > max_w);
    if (pos > 0) {
      s.replace(0, pos, "...");
    }
  }
  return s;
}

void Source::Draw() {
  werase(w_);
  wattrset(w_, A_NORMAL);
  if (item_ == nullptr) {
    mvwprintw(w_, 0, 0, "Module instance source code appears here.");
    return;
  }
  const int win_h = getmaxy(w_);
  const int win_w = getmaxx(w_);
  const int max_digits = num_decimal_digits(scroll_row_ + win_h);
  SetColor(w_, kSourceHeaderPair);
  mvwaddnstr(w_, 0, 0, GetHeader(win_w).c_str(), win_w);

  if (lines_.empty()) {
    SetColor(w_, kSourceTextPair);
    mvwprintw(w_, 1, 0, "Unable to open file");
    return;
  }
  for (int y = 1; y < win_h; ++y) {
    int line_idx = y - 1 + scroll_row_;
    if (line_idx >= lines_.size()) break;
    if (line_idx == line_idx_) wattron(w_, A_REVERSE);
    const int line_num = line_idx + 1;
    const int line_num_size = num_decimal_digits(line_num);
    SetColor(w_, kSourceLineNrPair);
    mvwprintw(w_, y, max_digits - line_num_size, "%d", line_num);
    SetColor(w_, kSourceTextPair);
    waddch(w_, ' ');
    const auto &s = lines_[line_idx].text;
    const auto &keywords = tokenizer_.Keywords(line_idx);
    const auto &identifiers = tokenizer_.Identifiers(line_idx);
    const auto &comments = tokenizer_.Comments(line_idx);
    bool in_keyword = false;
    bool in_comment = false;
    bool in_identifier = false;
    int k_idx = 0;
    int c_idx = 0;
    int id_idx = 0;
    for (int x = max_digits + 1; x < win_w; ++x) {
      int pos = x - max_digits - 1;
      if (pos >= s.size()) break;
      // Figure out if we have to switch to a new color
      if (!in_keyword && keywords.size() > k_idx &&
          keywords[k_idx].first == pos) {
        in_keyword = true;
        SetColor(w_, kSourceKeywordPair);
      } else if (!in_comment && comments.size() > c_idx &&
                 comments[c_idx].first == pos) {
        in_comment = true;
        SetColor(w_, kSourceCommentPair);
      } else if (!in_identifier && identifiers.size() > id_idx &&
                 identifiers[id_idx].first == pos) {
        in_identifier = true;
        SetColor(w_, kSourceIndentifierPair);
      }
      waddch(w_, s[pos]);
      // See if the color should be turned off.
      if (in_keyword && keywords[k_idx].second == pos) {
        in_keyword = false;
        k_idx++;
        SetColor(w_, kSourceTextPair);
      } else if (in_comment && comments[c_idx].second == pos) {
        in_comment = false;
        c_idx++;
        SetColor(w_, kSourceTextPair);
      } else if (in_identifier &&
                 (identifiers[id_idx].first +
                  identifiers[id_idx].second.size() - 1) == pos) {
        in_identifier = false;
        id_idx++;
        SetColor(w_, kSourceTextPair);
      }
    }
    wattroff(w_, A_REVERSE);
  }
  // TODO: remove
  // if (state_.item->VpiType() == vpiModule) {
  //  auto m = reinterpret_cast<UHDM::module *>(state_.item);
  //  auto item = state_.item;
  //  mvwprintw(w_, 0, 0, "|%d %d %d %d %d|", item->VpiLineNo(),
  //  item->VpiEndLineNo(),
  //            item->VpiColumnNo(), item->VpiEndColumnNo(), m->VpiDefLineNo());
  //  SetColor(w_, kTooltipKeyPair);
  //  mvwprintw(w_, 1, 0, "%s", m->VpiDefFile().c_str());
  //}
}

void Source::UIChar(int ch) {
  switch (ch) {
  case 'u': {
    // TODO: Go up in scope.
    break;
  }
  default: Panel::UIChar(ch);
  }
}

std::pair<int, int> Source::ScrollArea() {
  // Account for the header.
  int h, w;
  getmaxyx(w_, h, w);
  return {h - 1, w};
}

bool Source::TransferPending() { return false; }

void Source::SetItem(UHDM::BaseClass *item, bool open_def) {
  item_ = item;
  // Read all lines. TODO: Handle huge files.
  lines_.clear();
  int line_num = 0;
  switch (item->VpiType()) {
  case vpiModule: {
    auto m = reinterpret_cast<UHDM::module *>(item);
    if (open_def) {
      current_file_ = m->VpiDefFile();
      line_num = m->VpiDefLineNo();
    } else {
      current_file_ = m->VpiFile();
      line_num = m->VpiLineNo();
    }
    break;
  }
  case vpiGenScopeArray: {
    auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item);
    current_file_ = ga->VpiFile();
    line_num = ga->VpiLineNo();
    break;
  }
  default:
    // Do nothing for unknown types.
    return;
  }
  line_idx_ = line_num - 1;
  tokenizer_ = SimpleTokenizer(); // Clear out old info.
  std::ifstream is(current_file_);
  if (is.fail()) return; // Draw function handles file open issues.
  std::string s;
  while (std::getline(is, s)) {
    trim_string(s);
    tokenizer_.ProcessLine(s);
    lines_.push_back({.text = std::move(s)});
  }
  // Scroll to module definition, attempt to place the line at 1/3rd the screen.
  const int win_h = getmaxy(w_) - 1; // Account for header
  const int lines_remaining = lines_.size() - line_idx_ - 1;
  if (lines_.size() <= win_h - 1) {
    // If all lines fit on the screen, accounting for the header, then just
    // don't scroll.
    scroll_row_ = 0;
  } else if (line_idx_ < win_h / 3) {
    // Go as far down to the 1/3rd line as possible.
    scroll_row_ = 0;
  } else if (lines_remaining < 2 * win_h / 3) {
    // If there are aren't many lines after the current location, scroll as far
    // up as possible.
    scroll_row_ = lines_.size() - win_h;
  } else {
    scroll_row_ = line_idx_ - win_h / 3;
  }
}

} // namespace sv
