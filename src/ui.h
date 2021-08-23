#pragma once

#include "absl/time/time.h"
#include "design_tree_panel.h"
#include "source_panel.h"
#include "text_input.h"
#include "waves_panel.h"
#include "workspace.h"
#include <memory>

namespace sv {

class UI {
 public:
  UI();
  ~UI();

  void EventLoop();

 private:
  void DrawPanes(bool resize);

  int wave_pos_y_ = -1;
  int src_pos_x_ = -1;
  int term_w_;
  int term_h_;
  std::vector<int> tmp_ch; // TODO: remove
  absl::Time last_ch;

  std::unique_ptr<DesignTreePanel> design_tree_;
  std::unique_ptr<SourcePanel> source_;
  std::unique_ptr<WavesPanel> waves_;
  // A list of all panels makes it easy to cycle focus.
  std::vector<Panel *> panels_;
  int focused_panel_idx_ = 0;

  // Search state.
  bool searching_ = false;
  TextInput search_box_;
};

} // namespace sv
