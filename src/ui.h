#ifndef _SRC_UI_H_
#define _SRC_UI_H_

#include "hierarchy.h"
#include "source.h"
#include "waves.h"
#include "absl/time/time.h"
#include <memory>
#include <uhdm.h>

namespace sv {

class UI {
 public:
  UI();
  ~UI();

  void EventLoop();
  void SetDesign(UHDM::design *d);

 private:
  void DrawPanes(bool resize);

  int wave_pos_y_;
  int src_pos_x_;
  int term_w_;
  int term_h_;
  std::vector<int> tmp_ch; // TODO: remove
  absl::Time last_ch;

  std::unique_ptr<Hierarchy> hierarchy_;
  std::unique_ptr<Source> source_;
  std::unique_ptr<Waves> waves_;
  Panel *focused_panel_ = nullptr;
  Panel *prev_focused_panel_ = nullptr;
};

} // namespace sv
#endif // _SRC_UI_H_
