#ifndef _SRC_UI_H_
#define _SRC_UI_H_

#include "hierarchy.h"
#include "source.h"
#include "waves.h"
#include <memory>
#include <surelog/surelog.h>

namespace sv {

class UI {
 public:
  UI();
  ~UI();

  void EventLoop();
  void SetDesign(SURELOG::Design *d);

 private:
  void DrawPanes(bool resize);

  int wave_pos_y_;
  int src_pos_x_;
  int term_w_;
  int term_h_;

  std::unique_ptr<Hierarchy> hierarchy_;
  std::unique_ptr<Source> source_;
  std::unique_ptr<Waves> waves_;
  Panel *focused_panel_ = nullptr;
};

} // namespace sv
#endif // _SRC_UI_H_
