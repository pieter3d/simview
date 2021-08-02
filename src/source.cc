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
  auto &item = state_.item;
  std::string type;
  switch (item->VpiType()) {
  case vpiModule: {
    auto m = reinterpret_cast<UHDM::module *>(item);
    type = m->VpiFullName();
    break;
  }
  case vpiGenScopeArray: {
    auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item);
    type = ga->VpiFullName();
    break;
  }
  default:
    type = "Unknown type: " + std::to_string(item->VpiType());
    break;
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
  if (state_.item == nullptr) {
    mvwprintw(w_, 0, 0, "Module instance source code appears here.");
    return;
  }
  const int win_h = getmaxy(w_);
  const int win_w = getmaxx(w_);
  const int max_digits = num_decimal_digits(ui_row_scroll_ + win_h);
  SetColor(w_, kSourceHeaderPair);
  mvwaddnstr(w_, 0, 0, GetHeader(win_w).c_str(), win_w);
  int min_line = state_.item->VpiLineNo();
  int max_line = state_.item->VpiEndLineNo();

  if (lines_.empty()) {
    SetColor(w_, kSourceTextPair);
    mvwprintw(w_, 1, 0, "Unable to open file");
    return;
  }
  for (int y = 1; y < win_h; ++y) {
    int line_idx = y - 1 + ui_row_scroll_;
    if (line_idx >= lines_.size()) break;
    const int line_num = line_idx + 1;
    const int line_num_size = num_decimal_digits(line_num);
    const bool active_line = line_num >= min_line && line_num <= max_line;
    SetColor(w_, active_line ? kSourceLineNrPair : kSourceLineNrPair);
    mvwprintw(w_, y, max_digits - line_num_size, "%d", line_num);
    SetColor(w_, active_line ? kSourceTextPair : kSourceTextPair);
    waddch(w_, ' ');
    waddnstr(w_, lines_[line_idx].text.c_str(), win_w - max_digits - 1);
  }
  // TODO: remove
  //if (state_.item->VpiType() == vpiModule) {
  //  auto m = reinterpret_cast<UHDM::module *>(state_.item);
  //  auto item = state_.item;
  //  mvwprintw(w_, 0, 0, "|%d %d %d %d %d|", item->VpiLineNo(), item->VpiEndLineNo(),
  //            item->VpiColumnNo(), item->VpiEndColumnNo(), m->VpiDefLineNo());
  //  SetColor(w_, kTooltipKeyPair);
  //  mvwprintw(w_, 1, 0, "%s", m->VpiDefFile().c_str());
  //}
}

void Source::UIChar(int ch) {}

bool Source::TransferPending() { return false; }

void Source::SetItem(UHDM::BaseClass *item, bool open_def) {
  state_.item = item;
  // Read all lines. TODO: Handle huge files.
  lines_.clear();
  switch (item->VpiType()) {
  case vpiModule: {
    auto m = reinterpret_cast<UHDM::module *>(item);
    if (open_def) { 
      const std::string &def_name = m->VpiDefName();
      if (module_defs_.find(def_name) == module_defs_.end()) {
        // Find the module definition in the UHDB.
        for (auto &candidate_module: *design_->AllModules()) {
          if (def_name == candidate_module->VpiDefName()) {
            module_defs_[def_name] = candidate_module;
            break;
          }
        }
      }
      auto def = module_defs_[def_name];
      current_file_ = def->VpiFile();
      state_.line_num = def->VpiLineNo();
      // TODO: Seems to be a bug in UHDM, def file info is blank.
    } else {
      current_file_ = m->VpiFile();
      state_.line_num = m->VpiLineNo();
    }
    break;
  }
  case vpiGenScopeArray: {
    auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item);
    current_file_ = ga->VpiFile();
    state_.line_num = ga->VpiLineNo();
    break;
  }
  default:
    // Do nothing for unknown types.
    return;
  }
  std::ifstream is(current_file_);
  if (is.fail()) return; // Draw function handles file open issues.
  std::string s;
  while (std::getline(is, s)) {
    trim_string(s);
    lines_.push_back({.text = std::move(s)});
  }
  // Scroll to module definition
  ui_line_index_ = 0;
  ui_col_scroll_ = 0;
  ui_row_scroll_ = state_.line_num - 1;
}

} // namespace sv
