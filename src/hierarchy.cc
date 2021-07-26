#include "hierarchy.h"
#include "SourceCompile/VObjectTypes.h"
#include "absl/strings/str_format.h"
#include "color.h"

namespace sv {
namespace {
std::string strip_worklib(const std::string &s) {
  const int lib_delimieter_pos = s.find('@');
  if (lib_delimieter_pos == std::string::npos) return s;
  return s.substr(lib_delimieter_pos + 1);
}

bool has_sub_instances(SURELOG::ModuleInstance *inst) {
  // TODO: Remove once the list of expandable types is figured out.
  return inst->getNbChildren() != 0;
  bool ret = false;
  for (auto &sub : inst->getAllSubInstances()) {
    if (sub->getType() == slModule_instantiation) return true;
  }
  return ret;
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
    const int instance_idx = y + ui_row_scroll_;
    if (instance_idx >= instances_.size()) break;
    if (y == ui_line_index_) wattron(w_, A_REVERSE);
    auto line = instances_[instance_idx];
    auto inst_name = strip_worklib(line.instance->getInstanceName());
    auto mod_name = strip_worklib(line.instance->getModuleName());
    char exp = line.expandable ? (line.expanded ? '-' : '+') : ' ';
    std::string indent(line.depth, ' ');
    std::string s = indent + exp + inst_name + ' ' + mod_name + ' ' +
                    // TODO: remove type debug
                    absl::StrFormat("(type = %d)", line.instance->getType());
    max_string_len = std::max(max_string_len, static_cast<int>(s.size()));
    // printf("DBG:%d - %s\n\r", instance_idx, s.c_str());
    // mvwprintw(w_, y, 0, "%d - %s", instance_idx, s.c_str());
    int expand_pos = indent.size();
    int inst_pos = expand_pos + 1;
    int mod_pos = inst_pos + inst_name.size() + 1;
    for (int j = 0; j < s.size(); ++j) {
      const int x = j - ui_col_scroll_;
      if (x < 0) continue;
      if (x >= win_w) break;
      if (x == 0 && ui_col_scroll_ != 0 && j >= expand_pos) {
        // Show an overflow character on the left edge if the ui has been
        // scrolled horizontally.
        wcolor_set(w_, kOverflowTextPair, nullptr);
        mvwaddch(w_, y, x, '<');
      } else if (x == win_w - 1 && j < s.size() - 1) {
        // Replace the last character with an overflow indicator if the line
        // extends beyond the window width.
        wcolor_set(w_, kOverflowTextPair, nullptr);
        mvwaddch(w_, y, x, '>');
      } else {
        if (j >= mod_pos) {
          wcolor_set(w_, kHierModulePair, nullptr);
        } else if (j >= inst_pos) {
          wcolor_set(w_, kHierInstancePair, nullptr);
        } else if (j == expand_pos && line.expandable) {
          wcolor_set(w_, kHierExpandPair, nullptr);
        }
        mvwaddch(w_, y, x, s[j]);
      }
    }
    wattrset(w_, A_NORMAL);
  }
  ui_max_col_scroll_ = max_string_len - win_w;
}

void Hierarchy::ToggleExpand() {
  const int idx = ui_line_index_ + ui_row_scroll_;
  auto inst = instances_[idx];
  auto first = instances_.cbegin() + idx;
  if (!inst.expandable) return;
  if (inst.expanded) {
    // Delete everything under the current index that has greater depth.
    auto last = first + 1;
    while (last != instances_.cend() && last->depth > inst.depth) {
      last++;
    }
    instances_.erase(first + 1, last);
  } else {
    int idx = 0;
    std::vector<InstanceLine> new_lines;
    for (auto &sub : inst.instance->getAllSubInstances()) {
      // Skip anything that isn't a module.
      // TODO: uncomment
      // if (sub->getType() != slModule_instantiation) continue;
      new_lines.push_back({.instance = sub,
                           .depth = inst.depth + 1,
                           .index_in_parent = idx++,
                           .expandable = has_sub_instances(sub)});
    }
    instances_.insert(first + 1, new_lines.cbegin(), new_lines.cend());
  }
  instances_[idx].expanded = !instances_[idx].expanded;
}

void Hierarchy::UIChar(int ch) {
  const int data_idx = ui_line_index_ + ui_row_scroll_;
  int win_h = getmaxy(w_);
  switch (ch) {
  case 0x20: // enter
  case 0xa:  // space
    ToggleExpand();
    break;
  case 0x104: // left
    if (ui_col_scroll_ > 0) ui_col_scroll_--;
    break;
  case 0x105: // right
    if (ui_col_scroll_ < ui_max_col_scroll_) ui_col_scroll_++;
    break;
  case 0x103: // up
    if (ui_line_index_ == 0 && ui_row_scroll_ != 0) {
      ui_row_scroll_--;
    } else if (ui_line_index_ > 0) {
      ui_line_index_--;
    }
    break;
  case 0x102: // down
    if (data_idx < instances_.size() - 1) {
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
        std::min(static_cast<int>(instances_.size()) - (ui_row_scroll_ + win_h),
                 win_h - 2);
    ui_row_scroll_ += step;
    ui_line_index_ = std::max(2, ui_line_index_ - step);
    break;
  }
  case 'g':   // vim style
  case 0x217: // Ctrl Home
    ui_row_scroll_ = 0;
    ui_line_index_ = 0;
    break;
  case 'G':   // vim style
  case 0x212: // Ctrl End
    if (instances_.size() > win_h) {
      ui_row_scroll_ = instances_.size() - win_h;
      ui_line_index_ = win_h - 1;
    } else {
      ui_line_index_ = instances_.size();
    }
    break;
  }
}

void Hierarchy::SetDesign(SURELOG::Design *d) {
  design_ = d;
  int idx = 0;
  for (auto &top : d->getTopLevelModuleInstances()) {
    instances_.push_back({.instance = top,
                          .depth = 0,
                          .index_in_parent = idx++,
                          .expandable = has_sub_instances(top)});
  }
  ui_line_index_ = 0;
  ui_row_scroll_ = 0;
  // First instance is pre-expanded for usability if there is just one.
  if (instances_.size() == 1) ToggleExpand();
}

} // namespace sv
