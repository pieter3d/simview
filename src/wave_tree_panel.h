#pragma once

#include "tree_panel.h"
#include "wave_tree_item.h"
#include <memory>

namespace sv {

class WaveTreePanel : public TreePanel {
 public:
  WaveTreePanel();
  void UIChar(int ch) override;
  std::string Tooltip() const override;
  bool Searchable() const override { return true; }

 private:
  std::unique_ptr<WaveTreeItem> root_;
};

} // namespace sv