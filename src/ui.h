#ifndef _SRC_UI_H_
#define _SRC_UI_H_

namespace sv {

class UI {
 public:
  UI();
  ~UI();

  void EventLoop();

 private:
  void DrawPanes();

  int wave_pos_y_;
  int src_pos_x_;
  int term_w_;
  int term_h_;
};
} // namespace sv
#endif // _SRC_UI_H_
