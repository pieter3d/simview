#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "panel.h"
#include <uhdm/headers/BaseClass.h>
#include <uhdm/headers/design.h>
#include <vector>

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;
  bool TransferPending() override;
  std::string Tooltip() const override { return ""; }
  void SetItem(UHDM::BaseClass *item, bool open_def = false);
  void SetDesign(UHDM::design *d) { design_ = d; }

 private:
  struct State {
    UHDM::BaseClass *item = nullptr;
    int line_num = 0;
  };
  struct SourceLine {
    std::string text;
  };

  std::string GetHeader(int max_w);

  int ui_col_scroll_ = 0;
  int ui_row_scroll_ = 0;
  int ui_line_index_ = 0;
  std::vector<SourceLine> lines_;
  std::unordered_map<std::string, UHDM::module *> module_defs_;
  State state_;
  std::string current_file_;
  UHDM::design *design_ = nullptr;
};

} // namespace sv
#endif // _SRC_SOURCE_H_
