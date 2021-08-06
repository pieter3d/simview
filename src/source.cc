#include "source.h"
#include "color.h"
#include "utils.h"
#include <curses.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <uhdm/headers/constant.h>
#include <uhdm/headers/gen_scope.h>
#include <uhdm/headers/gen_scope_array.h>
#include <uhdm/headers/module.h>
#include <uhdm/headers/param_assign.h>
#include <uhdm/headers/parameter.h>
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
    const bool active = line_num >= start_line_ && line_num <= end_line_;
    const int text_color = active ? kSourceTextPair : kSourceInactivePair;
    SetColor(w_, kSourceLineNrPair);
    // Fill the space before the line number with blanks so that it has the
    // right background color.
    for (int x = 0; x < max_digits - line_num_size; ++x) {
      mvwaddch(w_, y, x, ' ');
    }
    mvwprintw(w_, y, max_digits - line_num_size, "%d", line_num);
    SetColor(w_, text_color);
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
      const int pos = x - max_digits - 1;
      if (pos >= s.size()) break;
      // Figure out if we have to switch to a new color
      if (active && !in_keyword && keywords.size() > k_idx &&
          keywords[k_idx].first == pos) {
        in_keyword = true;
        SetColor(w_, kSourceKeywordPair);
      } else if (!in_comment && comments.size() > c_idx &&
                 comments[c_idx].first == pos) {
        // Highlight inactive comments still.
        in_comment = true;
        SetColor(w_, kSourceCommentPair);
      } else if (active && !in_identifier && identifiers.size() > id_idx &&
                 identifiers[id_idx].first == pos) {
        in_identifier = true;
        if (instances_.find(identifiers[id_idx].second) != instances_.end()) {
          SetColor(w_, kSourceInstancePair);
        } else if (nets_.find(identifiers[id_idx].second) != nets_.end()) {
          SetColor(w_, kSourceIdentifierPair);
        } else if (params_.find(identifiers[id_idx].second) != params_.end()) {
          SetColor(w_, kSourceParamPair);
        }
      }
      waddch(w_, s[pos]);
      // See if the color should be turned off.
      if (in_keyword && keywords[k_idx].second == pos) {
        in_keyword = false;
        k_idx++;
        SetColor(w_, text_color);
      } else if (in_comment && comments[c_idx].second == pos) {
        in_comment = false;
        c_idx++;
        SetColor(w_, text_color);
      } else if (in_identifier &&
                 (identifiers[id_idx].first +
                  identifiers[id_idx].second.size() - 1) == pos) {
        in_identifier = false;
        id_idx++;
        SetColor(w_, text_color);
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
  case 'u':
    // TODO: Go up in scope.
    break;
  }
  Panel::UIChar(ch);
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
  // Clear out old info.
  lines_.clear();
  tokenizer_ = SimpleTokenizer();
  nets_.clear();
  instances_.clear();
  int line_num = 0;

  // This lamda recurses through all generate blocks in the item, adding any
  // nets and instances to the list of navigable things.
  std::function<void(UHDM::gen_scope_array *)> recurse_genblocks =
      [&](UHDM::gen_scope_array *ga) {
        if (ga == nullptr) return;
        if (ga->Gen_scopes() == nullptr) return;
        auto &g = (*ga->Gen_scopes())[0];
        if (g->Nets() != nullptr) {
          for (auto &n : *g->Nets()) {
            nets_[n->VpiName()] = n;
          }
        }
        if (g->Variables() != nullptr) {
          for (auto &v : *g->Variables()) {
            nets_[v->VpiName()] = v;
          }
        }
        if (g->Modules() != nullptr) {
          for (auto &sub : *g->Modules()) {
            instances_[sub->VpiName()] = sub;
          }
        }
        if (g->Gen_scope_arrays() != nullptr) {
          for (auto &sub_ga : *g->Gen_scope_arrays()) {
            recurse_genblocks(sub_ga);
          }
        }
      };

  switch (item->VpiType()) {
  case vpiModule: {
    auto m = reinterpret_cast<UHDM::module *>(item);
    // Make a map of all net names -> net objects.
    if (m->Nets() != nullptr) {
      for (auto &n : *m->Nets()) {
        nets_[n->VpiName()] = n;
      }
    }
    if (m->Variables() != nullptr) {
      for (auto &v : *m->Variables()) {
        nets_[v->VpiName()] = v;
      }
    }
    if (m->Param_assigns() != nullptr) {
      for (auto &pa : *m->Param_assigns()) {
        // Elaborated designs should only have this type of assignment.
        if (pa->Lhs()->VpiType() != vpiParameter) continue;
        if (pa->Rhs()->VpiType() != vpiConstant) continue;
        auto p = reinterpret_cast<const UHDM::parameter *>(pa->Lhs());
        auto c = reinterpret_cast<const UHDM::constant *>(pa->Rhs());
        params_[p->VpiName()] = c->VpiDecompile();
      }
    }
    if (open_def) {
      // VpiDefFile isn't super useful here, still need the definition to get
      // the start and end line number.
      const std::string &def_name = m->VpiDefName();
      if (module_defs_.find(def_name) == module_defs_.end()) {
        // Find the module definition in the UHDB.
        for (auto &candidate_module : *workspace_.design->AllModules()) {
          if (def_name == candidate_module->VpiDefName()) {
            module_defs_[def_name] = candidate_module;
            break;
          }
        }
      }
      auto def = module_defs_[def_name];
      current_file_ = def->VpiFile();
      line_num = def->VpiLineNo();
      start_line_ = def->VpiLineNo();
      end_line_ = def->VpiEndLineNo();
      // Add all instances in this module as navigable instances.
      if (m->Modules() != nullptr) {
        for (auto &sub : *m->Modules()) {
          instances_[sub->VpiName()] = sub;
        }
      }
      if (m->Gen_scope_arrays() != nullptr) {
        for (auto *ga : *m->Gen_scope_arrays()) {
          recurse_genblocks(ga);
        }
      }
      // TODO: recurse through
    } else {
      current_file_ = m->VpiFile();
      line_num = m->VpiLineNo();
      start_line_ = m->VpiLineNo();
      end_line_ = m->VpiEndLineNo();
      // Add the instance identifier to the list of navigable instances.
      instances_[m->VpiName()] = m;
      // If showing the instance, also collect the nets of owning instance.
      // This will allow nets in the port connections to be navigable.
      if (m->VpiParent() != nullptr && m->VpiParent()->VpiType() == vpiModule) {
        auto parent = reinterpret_cast<const UHDM::module *>(m->VpiParent());
        if (parent->Nets() != nullptr) {
          for (auto &n : *parent->Nets()) {
            nets_[n->VpiName()] = n;
          }
        }
      }
    }
    break;
  }
  case vpiGenScopeArray: {
    auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item);
    current_file_ = ga->VpiFile();
    line_num = ga->VpiLineNo();
    start_line_ = ga->VpiLineNo();
    end_line_ = ga->VpiEndLineNo();
    recurse_genblocks(ga);
    break;
  }
  default:
    // Do nothing for unknown types.
    return;
  }
  line_idx_ = line_num - 1;
  // Read all lines. TODO: Handle huge files.
  std::ifstream is(current_file_);
  if (is.fail()) return; // Draw function handles file open issues.
  std::string s;
  while (std::getline(is, s)) {
    trim_string(s);
    tokenizer_.ProcessLine(s);
    lines_.push_back({.text = std::move(s)});
  }
  // Scroll to module definition, attempt to place the line at 1/3rd the
  // screen.
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
    // If there are aren't many lines after the current location, scroll as
    // far up as possible.
    scroll_row_ = lines_.size() - win_h;
  } else {
    scroll_row_ = line_idx_ - win_h / 3;
  }
}

} // namespace sv
