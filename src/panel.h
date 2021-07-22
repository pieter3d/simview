#ifndef _SRC_PANEL_H_
#define _SRC_PANEL_H_

#include <ncurses.h>

namespace sv {

class Panel {
 public:
  Panel(WINDOW *w) : w_(w) {}
  virtual ~Panel() { delwin(w_); }
  virtual void Draw() const = 0;
  WINDOW *Window() const { return w_; }

 protected:
  WINDOW *w_;
};
} // namespace sv

#endif // _SRC_PANEL_H_
