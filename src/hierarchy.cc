#include "hierarchy.h"
#include "absl/strings/str_format.h"
#include "color.h"
#include "utils.h"
#include <algorithm>
#include <iostream>
#include <uhdm/headers/BaseClass.h>
#include <uhdm/headers/gen_scope.h>
#include <uhdm/headers/gen_scope_array.h>
#include <uhdm/headers/module.h>

namespace sv {
namespace {

// Limit number of instances dumped into the UI at once.
constexpr int kMaxExpandInstances = 100;

std::vector<UHDM::BaseClass *> get_subs(const UHDM::BaseClass *item) {
  std::vector<UHDM::BaseClass *> subs;
  if (item->VpiType() == vpiModule) {
    auto m = reinterpret_cast<const UHDM::module *>(item);
    if (m->Modules() != nullptr) {
      subs.insert(subs.end(), m->Modules()->cbegin(), m->Modules()->cend());
    }
    if (m->Gen_scope_arrays() != nullptr) {
      subs.insert(subs.end(), m->Gen_scope_arrays()->cbegin(),
                  m->Gen_scope_arrays()->cend());
    }
    // TODO: Other stuff. Tasks & functions.
    // TODO: How do module arrays work?
  } else if (item->VpiType() == vpiGenScopeArray) {
    // TODO: What to do if there is 0 or 2+ GenScopes in here??
    auto ga = reinterpret_cast<const UHDM::gen_scope_array *>(item);
    auto g = (*ga->Gen_scopes())[0];
    if (g->Modules() != nullptr) {
      subs.insert(subs.end(), g->Modules()->cbegin(), g->Modules()->cend());
    }
    if (g->Gen_scope_arrays() != nullptr) {
      subs.insert(subs.end(), g->Gen_scope_arrays()->cbegin(),
                  g->Gen_scope_arrays()->cend());
    }
  }
  // TODO: Also offer lexical sort?
  std::stable_sort(subs.begin(), subs.end(),
                   [](UHDM::BaseClass *a, UHDM::BaseClass *b) {
                     return a->VpiLineNo() < b->VpiLineNo();
                   });

  return subs;
}

bool is_expandable(const UHDM::BaseClass *item) {
  // TODO: This could be optimized without actually creating the array, but
  // would be lots of duplicated code.
  return !get_subs(item).empty();
}

} // namespace

Hierarchy::Hierarchy(WINDOW *w) : Panel(w) {}

void Hierarchy::Draw() {
  werase(w_);
  int win_w, win_h;
  getmaxyx(w_, win_h, win_w);
  // If the window was resized and the ui line position is now hidden, move it
  // up.
  if (ui_line_index_ >= win_h) {
    const int shift = ui_line_index_ - (win_h - 1);
    ui_line_index_ -= shift;
    ui_row_scroll_ += shift;
  }
  int max_string_len = 0;
  for (int y = 0; y < win_h; ++y) {
    const int entry_idx = y + ui_row_scroll_;
    if (entry_idx >= entries_.size()) break;
    if (y == ui_line_index_) wattron(w_, A_REVERSE);
    auto entry = entries_[entry_idx];
    auto info = entry_info_[entry];
    std::string indent(info.depth, ' ');
    if (info.more_idx != 0) {
      SetColor(w_, kHierShowMorePair);
      mvwprintw(w_, y, 0, "%s...more...", indent.c_str());
    } else {
      std::string def_name = StripWorklib(entry->VpiDefName());
      if (entry->VpiType() == vpiGenScopeArray) {
        def_name = "[generate]";
      }
      // VpiName generally doesn't contain the worklib, but it does for the top
      // level modules.
      auto inst_name = StripWorklib(entry->VpiName());
      char exp = info.expandable ? (info.expanded ? '-' : '+') : ' ';
      std::string s = indent + exp + inst_name + ' ' + def_name;
      max_string_len = std::max(max_string_len, static_cast<int>(s.size()));
      int expand_pos = indent.size();
      int inst_pos = expand_pos + 1;
      int type_pos = inst_pos + inst_name.size() + 1;
      for (int j = 0; j < s.size(); ++j) {
        const int x = j - ui_col_scroll_;
        if (x < 0) continue;
        if (x >= win_w) break;
        if (x == 0 && ui_col_scroll_ != 0 && j >= expand_pos) {
          // Show an overflow character on the left edge if the ui has been
          // scrolled horizontally.
          SetColor(w_, kOverflowTextPair);
          mvwaddch(w_, y, x, '<');
        } else if (x == win_w - 1 && j < s.size() - 1) {
          // Replace the last character with an overflow indicator if the line
          // extends beyond the window width.
          SetColor(w_, kOverflowTextPair);
          mvwaddch(w_, y, x, '>');
        } else {
          if (j >= type_pos) {
            SetColor(w_, (entry->VpiType() != vpiModule) ? kHierOtherPair
                                                         : kHierModulePair);
          } else if (j >= inst_pos) {
            SetColor(w_, kHierInstancePair);
          } else if (j == expand_pos && info.expandable) {
            SetColor(w_, kHierExpandPair);
          }
          mvwaddch(w_, y, x, s[j]);
        }
      }
    }
    wattrset(w_, A_NORMAL);
  }
  ui_max_col_scroll_ = max_string_len - win_w;
}

void Hierarchy::ToggleExpand() {
  const int idx = ui_line_index_ + ui_row_scroll_;
  const auto entry_it = entries_.cbegin() + idx;
  if (entries_.empty()) return;
  auto &info = entry_info_[*entry_it];
  if (info.more_idx != 0) {
    int stopped_pos = info.more_idx;
    info.more_idx = 0;
    std::vector<UHDM::BaseClass *> new_entries;
    const auto parent_subs = get_subs(info.parent);
    for (int i = stopped_pos + 1; i < parent_subs.size(); ++i) {
      auto new_sub = parent_subs[i];
      int more_idx = (i - stopped_pos) >= kMaxExpandInstances ? i : 0;
      new_entries.push_back(new_sub);
      entry_info_[new_sub] = {.depth = info.depth,
                              .expandable = is_expandable(new_sub),
                              .more_idx = more_idx,
                              .parent = info.parent};
      if (more_idx != 0) break;
    }
    entries_.insert(entry_it + 1, new_entries.cbegin(), new_entries.cend());
    return;
  } else if (!info.expandable) {
    return;
  } else if (info.expanded) {
    // Delete everything under the current index that has greater depth.
    auto last = entry_it + 1;
    while (last != entries_.cend() && entry_info_[*last].depth > info.depth) {
      last++;
    }
    entries_.erase(entry_it + 1, last);
    info.expanded = false;
  } else {
    std::vector<UHDM::BaseClass *> new_entries;
    std::function<void(UHDM::BaseClass *, std::vector<UHDM::BaseClass *> &)>
        recurse_add_subs =
            [&](UHDM::BaseClass *item, std::vector<UHDM::BaseClass *> &list) {
              int sub_idx = 0;
              auto subs = get_subs(item);
              for (auto &sub : subs) {
                list.push_back(sub);
                // Create new UI info only if it doesn't already exist
                if (entry_info_.find(sub) == entry_info_.end()) {
                  int more_idx = sub_idx >= kMaxExpandInstances ? sub_idx : 0;
                  entry_info_[sub] = {.depth = entry_info_[item].depth + 1,
                                      .expandable = is_expandable(sub),
                                      .more_idx = more_idx,
                                      .parent = item};
                  sub_idx++;
                  if (more_idx != 0) break;
                } else if (entry_info_[sub].more_idx != 0) {
                  break;
                } else if (entry_info_[sub].expanded) {
                  recurse_add_subs(sub, list);
                }
              }
            };
    recurse_add_subs(*entry_it, new_entries);
    entries_.insert(entry_it + 1, new_entries.cbegin(), new_entries.cend());
    info.expanded = true;
    // Move the selection up a ways if the new lines are all hidden.
    const int win_h = getmaxy(w_);
    if (ui_line_index_ == win_h - 1) {
      const int lines_below = entries_.size() - ui_row_scroll_ - win_h;
      const int scroll_amt = std::min(lines_below, win_h / 3);
      ui_row_scroll_ += scroll_amt;
      ui_line_index_ -= scroll_amt;
    }
  }
}

void Hierarchy::UIChar(int ch) {
  const int data_idx = ui_line_index_ + ui_row_scroll_;
  int win_h = getmaxy(w_);
  switch (ch) {
  case 's': load_source_ = true; break;
  case 'u': {
    int current_depth = entry_info_[entries_[data_idx]].depth;
    int new_idx = data_idx - 1;
    if (current_depth != 0) {
      while (entry_info_[entries_[new_idx]].depth >= current_depth) {
        new_idx--;
      }
      int delta = data_idx - new_idx;
      ui_line_index_ -= delta;
      if (ui_line_index_ < 0) {
        ui_row_scroll_ += ui_line_index_;
        ui_line_index_ = 0;
      }
    }
    break;
  }
  case 0x20: // space
  case 0xd:  // enter
    ToggleExpand();
    break;
  case 'h':
  case 0x104: // left
    if (ui_col_scroll_ > 0) ui_col_scroll_--;
    break;
  case 'l':
  case 0x105: // right
    if (ui_col_scroll_ < ui_max_col_scroll_) ui_col_scroll_++;
    break;
  case 'k':
  case 0x103: // up
    if (ui_line_index_ == 0 && ui_row_scroll_ != 0) {
      ui_row_scroll_--;
    } else if (ui_line_index_ > 0) {
      ui_line_index_--;
    }
    break;
  case 'j':
  case 0x102: // down
    if (data_idx < entries_.size() - 1) {
      if (ui_line_index_ < win_h - 1) {
        ui_line_index_++;
      } else {
        ui_row_scroll_++;
      }
    }
    break;
  case 0x153: { // PgUp
    int step = std::min(ui_row_scroll_, win_h - 2);
    ui_row_scroll_ -= step;
    ui_line_index_ = std::min(win_h - 2, ui_line_index_ + step);
    break;
  }
  case 0x152: { // PgDn
    int step =
        std::min(static_cast<int>(entries_.size()) - (ui_row_scroll_ + win_h),
                 win_h - 2);
    ui_row_scroll_ += step;
    if (step == 0) {
      ui_line_index_ = win_h - 1;
    } else {
      ui_line_index_ = std::max(2, ui_line_index_ - step);
    }
    break;
  }
  case 'g':   // vim style
  case 0x217: // Ctrl Home
    ui_row_scroll_ = 0;
    ui_line_index_ = 0;
    break;
  case 'G':   // vim style
  case 0x212: // Ctrl End
    if (entries_.size() > win_h) {
      ui_row_scroll_ = entries_.size() - win_h;
      ui_line_index_ = win_h - 1;
    } else {
      ui_line_index_ = entries_.size();
    }
    break;
  }
}

bool Hierarchy::TransferPending() {
  if (load_source_) {
    load_source_ = false;
    return true;
  }
  return false;
}

UHDM::BaseClass *Hierarchy::ItemForSource() {
  return entries_[ui_line_index_ + ui_row_scroll_];
}

void Hierarchy::SetDesign(UHDM::design *d) {
  if (d == nullptr) return;

  design_ = d;
  for (auto &top : *d->TopModules()) {
    entries_.push_back(top);
    entry_info_[top].expandable = is_expandable(top);
  }
  // Put all top modules with sub instances on top.
  // Lexical sort within that.
  auto top_sorter = [](UHDM::BaseClass *a, UHDM::BaseClass *b) {
    auto &ma = reinterpret_cast<UHDM::module *&>(a);
    auto &mb = reinterpret_cast<UHDM::module *&>(b);
    bool a_has_subs = ma->Modules() != nullptr || ma->Gen_scope_arrays();
    bool b_has_subs = mb->Modules() != nullptr || mb->Gen_scope_arrays();
    if (a_has_subs == b_has_subs) {
      return a->VpiName() < b->VpiName();
    } else {
      return a_has_subs;
    }
  };
  std::stable_sort(entries_.begin(), entries_.end(), top_sorter);

  ui_line_index_ = 0;
  ui_row_scroll_ = 0;
  // First instance is pre-expanded for usability if there is just one.
  if (entries_.size() == 1) ToggleExpand();
}

std::string Hierarchy::Tooltip() const { return "s:open source u:up scope"; }

} // namespace sv
