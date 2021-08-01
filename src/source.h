#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include <Design/ModuleInstance.h>
#include "panel.h"

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  std::string Tooltip() const override { return ""; }
  void SetInstance(SURELOG::ModuleInstance *inst);

 private:
  struct State {
    SURELOG::ModuleInstance *instance = nullptr;
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
};

} // namespace sv
#endif // _SRC_SOURCE_H_
