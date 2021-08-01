#include "hierarchy.h"
#include "BaseClass.h"
#include "absl/strings/str_format.h"
#include "color.h"
#include <algorithm>
#include <iostream>
#include <uhdm/headers/gen_scope.h>
#include <uhdm/headers/gen_scope_array.h>
#include <uhdm/headers/module.h>

namespace sv {
namespace {

// Limit number of instances dumped into the UI at once.
constexpr int kMaxExpandInstances = 500;

std::string strip_worklib(const std::string &s) {
  const int lib_delimieter_pos = s.find('@');
  if (lib_delimieter_pos == std::string::npos) return s;
  return s.substr(lib_delimieter_pos + 1);
}

bool is_expandable(UHDM::BaseClass *item) {
  if (item->VpiType() == vpiModule) {
    auto m = reinterpret_cast<UHDM::module *>(item);
    return m->Task_funcs() != nullptr || m->Modules() != nullptr ||
           m->Gen_scope_arrays() != nullptr;
  } else if (item->VpiType() == vpiGenScopeArray) {
    auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item);
    // TODO: How could this ever not be 1?
    if (ga->Gen_scopes()->size() != 1) {
      std::cout << ga->VpiName();
      return true;
    } else {
      auto g = (*ga->Gen_scopes())[0];
      return g->Modules() != nullptr || g->Gen_scope_arrays() != nullptr;
    }
    return ga->Gen_scopes()->size() > 1;
  } else if (item->VpiType() == vpiGenScope) {
    auto g = reinterpret_cast<UHDM::gen_scope *>(item);
    return g->Modules() != nullptr || g->Gen_scope_arrays() != nullptr;
  }
  // if (scope->Scopes() == nullptr) return false;
  // for (auto &sub : *scope->Scopes()) {
  //  if (is_interesting(sub)) return true;
  //}
  return false;
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
      // auto type = entry->getType();
      // const bool type_is_generate = entry->VpiType() == vpiGenScopeArray;
      //    type == slGenerate_block || type ==
      //    slConditional_generate_construct;
      // if (type_is_generate) {
      //  type_name = "[generate]";
      //} else if (type == slModule_instantiation ||
      //           type == slModule_declaration) {
      //  // Module declarations in the hierarchy show up as root nodes, i.e.
      //  // individual tops.
      //  type_name = strip_worklib(entry->getModuleName());
      //} else {
      //  // For unknown stuff, just print the module name and the Surelog
      //  TypeID.
      //  // TODO: Remove once all supported things are handled.
      //  type_name = strip_worklib(entry->getModuleName()) + ' ' +
      //              absl::StrFormat("(type = %d)", type);
      //}
      std::string def_name = strip_worklib(entry->VpiDefName());
      if (entry->VpiType() == vpiGenScopeArray) {
        def_name = "[generate]";
      }
      // VpiName generally doesn't contain the worklib, but it does for the top
      // level modules.
      auto inst_name = strip_worklib(entry->VpiName());
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
    // int stopped_pos = info.more_idx;
    // info.more_idx = 0;
    // std::vector<SURELOG::ModuleInstance *> new_entries;
    // auto parent_subs = (*entry_it)->getParent()->getAllSubInstances();
    // for (int i = stopped_pos + 1; i < parent_subs.size(); ++i) {
    //  auto new_sub = parent_subs[i];
    //  if (!is_interesting(parent_subs[i])) continue;
    //  int more_idx = (i - stopped_pos) > kMaxExpandInstances ? i : 0;
    //  new_entries.push_back(new_sub);
    //  entry_info_[new_sub] = {.depth = info.depth,
    //                          .expandable = has_sub_instances(new_sub),
    //                          .more_idx = more_idx};
    //  if (more_idx != 0) break;
    //}
    // entries_.insert(entry_it + 1, new_entries.cbegin(), new_entries.cend());
    // return;
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
              std::vector<UHDM::BaseClass *> subs;
              if (item->VpiType() == vpiModule) {
                auto m = reinterpret_cast<UHDM::module *>(item);
                if (m->Modules() != nullptr) {
                  subs.insert(subs.end(), m->Modules()->cbegin(),
                              m->Modules()->cend());
                }
                if (m->Gen_scope_arrays() != nullptr) {
                  subs.insert(subs.end(), m->Gen_scope_arrays()->cbegin(),
                              m->Gen_scope_arrays()->cend());
                }
                // TODO: Other stuff. Tasks & functions.
                // TODO: How do module arrays work?
              } else if (item->VpiType() == vpiGenScopeArray) {
                // TODO: What to do if there is 0 or 2+ GenScopes in here??
                auto ga = reinterpret_cast<UHDM::gen_scope_array *>(item);
                auto g = (*ga->Gen_scopes())[0];
                if (g->Modules() != nullptr) {
                  subs.insert(subs.end(), g->Modules()->cbegin(),
                              g->Modules()->cend());
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
              for (auto &sub : subs) {
                list.push_back(sub);
                // Create new UI info only if it doesn't already exist
                if (entry_info_.find(sub) == entry_info_.end()) {
                  int more_idx = sub_idx > kMaxExpandInstances ? sub_idx : 0;
                  entry_info_[sub] = {.depth = entry_info_[item].depth + 1,
                                      .expandable = is_expandable(sub),
                                      .more_idx = more_idx};
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

UHDM::BaseClass *Hierarchy::InstanceForSource() {
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

std::string Hierarchy::Tooltip() const { return "s:open source"; }

} // namespace sv
