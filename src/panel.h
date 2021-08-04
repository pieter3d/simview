#ifndef _SRC_PANEL_H_
#define _SRC_PANEL_H_

#include <ncurses.h>
#include <string>

namespace sv {

class Panel {
 public:
  Panel(WINDOW *w) : w_(w) {}
  virtual ~Panel() { delwin(w_); }
  virtual void Draw() = 0;
  virtual void UIChar(int ch);
  virtual int NumLines() const = 0;
  virtual std::pair<int, int> ScrollArea();
  virtual bool TransferPending() = 0;
  virtual std::string Tooltip() const = 0;
  WINDOW *Window() const { return w_; }
  void Resized();

 protected:
  WINDOW *w_;
  int line_idx_ = 0;
  int scroll_row_ = 0;
};
} // namespace sv

#endif // _SRC_PANEL_H_
