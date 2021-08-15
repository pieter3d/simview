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
  bool ReceiveText(const std::string &s, bool preview) override;
  // Returns true if search_text_ is found in this panel. Searching must start
  // from the the current line_idx_ and search_start_col_ should be set
  // accordingly.
  virtual bool Search(bool search_down) { return false; }

 protected:
  virtual int NumLines() const = 0;
  virtual std::pair<int, int> ScrollArea();
  virtual void SetLineAndScroll(int l);
  WINDOW *w_;
  int line_idx_ = 0;
  int col_idx_ = 0;
  int scroll_row_ = 0;
  bool has_focus_ = false;
  // Search state
  bool search_preview_ = false;
  std::string search_text_;
  int search_start_col_ = 0;
  int search_orig_line_idx_ = -1;
  int search_orig_col_idx_ = -1;
};
} // namespace sv

#endif // _SRC_PANEL_H_
