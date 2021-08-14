#ifndef _SRC_PANEL_H_
#define _SRC_PANEL_H_

#include "text_input.h"
#include <ncurses.h>
#include <optional>
#include <string>

namespace sv {

class Panel : public TextReceiver {
 public:
  Panel(int h, int w, int row, int col);
  virtual ~Panel() { delwin(w_); }
  virtual void Draw() = 0;
  // Row,Col of the cursor position, if it should be shown.
  virtual std::optional<std::pair<int, int>> CursorLocation() const;
  // Handle keypress.
  virtual void UIChar(int ch);
  virtual std::string Tooltip() const { return ""; };
  virtual void SetFocus(bool f) { has_focus_ = f; }
  WINDOW *Window() const { return w_; }
  // Optional actions to take when the panel is resized, besides redraw.
  virtual void Resized();

 protected:
  virtual int NumLines() const = 0;
  virtual std::pair<int, int> ScrollArea();
  void SetLineAndScroll(int l);
  WINDOW *w_;
  int line_idx_ = 0;
  int scroll_row_ = 0;
  bool has_focus_ = false;
};
} // namespace sv

#endif // _SRC_PANEL_H_
