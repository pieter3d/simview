#include "hierarchy.h"
#include "absl/strings/str_format.h"

namespace sv {
namespace {
std::string strip_worklib(const std::string &s) {
  const int lib_delimieter_pos = s.find('@');
  if (lib_delimieter_pos == std::string::npos) return s;
  return s.substr(lib_delimieter_pos + 1);
}
} // namespace

Hierarchy::Hierarchy(WINDOW *w) : Panel(w) {}

void Hierarchy::Draw() const {
  werase(w_);
  int w, h;
  getmaxyx(w_, h, w);
  const int last_idx = std::min(static_cast<int>(instances_.size()),
                                instance_index_ + (h - ui_line_index_));
  int y = 0;
  std::string dbg;
  for (int i = instance_index_ - ui_line_index_; i < last_idx; ++i) {
    auto line = instances_[i];
    std::string s =
        absl::StrFormat("%c%s (%s)", line.expanded ? '-' : '+',
                        strip_worklib(line.instance->getInstanceName()),
                        strip_worklib(line.instance->getModuleName()));
    if (s.size() > w) {
      s.replace(w - 1, s.size() - (w - 1), ">");
    }
    if (y == ui_line_index_) wattron(w_, A_REVERSE);
    mvwprintw(w_, y++, 0, s.c_str());
    if (y == ui_line_index_) wattroff(w_, A_REVERSE);
  }
}

void Hierarchy::UIChar(int ch) {}

void Hierarchy::SetDesign(SURELOG::Design *d) {
  design_ = d;
  int idx = 0;
  for (auto &top : d->getTopLevelModuleInstances()) {
    instances_.push_back(
        {.instance = top, .depth = 0, .index_in_parent = idx++});
  }
  // First set of instances are pre-expanded for usability.
  // TODO: actually do this.
  current_instance_ = instances_[0].instance;
  ui_line_index_ = 0;
  instance_index_ = 0;
}

} // namespace sv
