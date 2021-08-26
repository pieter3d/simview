#pragma once

#include "signal_tree_item.h"
#include "tree_panel.h"
#include "wave_data.h"

namespace sv {

class WaveSignalsPanel : public TreePanel {
 public:
  WaveSignalsPanel();
  void Draw() override;
  void SetScope(const WaveData::SignalScope *s);
  bool Searchable() const override { return true; }
  virtual void UIChar(int ch) override;
  virtual std::string Tooltip() const override;
  std::pair<int, int> ScrollArea() const override;

 private:
  const WaveData::SignalScope *scope_;
  std::vector<SignalTreeItem> signals_;
  bool hide_inputs_ = false;
  bool hide_outputs_ = false;
  bool hide_inouts_ = false;
  bool hide_signals_ = false;
};

} // namespace sv
