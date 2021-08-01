#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "panel.h"
#include <uhdm/headers/BaseClass.h>
#include <vector>

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  std::string Tooltip() const override { return ""; }
  void SetItem(UHDM::BaseClass *item);

 private:
  struct State {
    UHDM::BaseClass *item = nullptr;
    int line_num = 0;
  };
  struct SourceLine {
    std::string text;
    int comment_start = -1;
    int comment_end = -1;
  };

  int ui_col_scroll_ = 0;
  int ui_row_scroll_ = 0;
  int ui_line_index_ = 0;
  std::vector<SourceLine> lines_;
  State state_;
  std::string current_file_;
};

} // namespace sv
#endif // _SRC_SOURCE_H_
