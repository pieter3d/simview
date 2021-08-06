#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "panel.h"
#include "simple_tokenizer.h"
#include "workspace.h"
#include <uhdm/headers/BaseClass.h>
#include <vector>

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w, Workspace &ws) : Panel(w), workspace_(ws) {}
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return lines_.size(); }
  bool TransferPending() override;
  std::string Tooltip() const override { return ""; }
  void SetItem(UHDM::BaseClass *item, bool open_def = false);
  std::pair<int, int> ScrollArea() override;

 private:
  struct State {
    UHDM::BaseClass *item = nullptr;
    int line_idx = 0;
  };
  struct SourceLine {
    std::string text;
  };

  std::string GetHeader(int max_w);

  std::vector<SourceLine> lines_;
  std::unordered_map<std::string, UHDM::module *> module_defs_;
  UHDM::BaseClass *item_ = nullptr;
  std::string current_file_;
  Workspace &workspace_;
  SimpleTokenizer tokenizer_;
  int start_line_ = 0;
  int end_line_ = 0;
};

} // namespace sv
#endif // _SRC_SOURCE_H_
