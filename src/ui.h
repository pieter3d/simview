#ifndef _SRC_UI_H_
#define _SRC_UI_H_

namespace sv {

class UI {
 public:
  UI();
  ~UI();

  void EventLoop();

 private:
  int wave_pos_y_;
  int src_pos_x_;
};
} // namespace sv
#endif // _SRC_UI_H_
