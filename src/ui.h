#ifndef _SRC_UI_H_
#define _SRC_UI_H_

#include "absl/time/time.h"
#include "design_tree_panel.h"
#include "source.h"
#include "text_input.h"
#include "waves.h"
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

  int wave_pos_y_;
  int src_pos_x_;
  int term_w_;
  int term_h_;
  std::vector<int> tmp_ch; // TODO: remove
  absl::Time last_ch;

  std::unique_ptr<DesignTreePanel> design_tree_;
  std::unique_ptr<Source> source_;
  std::unique_ptr<Waves> waves_;
  Panel *focused_panel_ = nullptr;
  Panel *prev_focused_panel_ = nullptr;

  bool searching_ = false;
  TextInput search_box_;
};

} // namespace sv
#endif // _SRC_UI_H_
