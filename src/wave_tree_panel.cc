#include "wave_tree_panel.h"
#include "workspace.h"

namespace sv {

WaveTreePanel::WaveTreePanel() {
  if (Workspace::Get().Waves() != nullptr) {
    root_ = std::make_unique<WaveTreeItem>(Workspace::Get().Waves()->Root());
    data_.AddRoot(root_.get());
  }
}

void WaveTreePanel::UIChar(int ch) {
  // TODO: do things.
  switch (ch) {
  default: TreePanel::UIChar(ch);
  }
}

std::string WaveTreePanel::Tooltip() const {
  // TODO: enhance.
  return "";
}

} // namespace sv
