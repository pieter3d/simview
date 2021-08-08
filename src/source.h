#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "panel.h"
#include "simple_tokenizer.h"
#include "workspace.h"
#include <deque>
#include <uhdm/headers/BaseClass.h>
#include <vector>

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w) : Panel(w) {}
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return lines_.size(); }
  std::string Tooltip() const override;
  void SetItem(const UHDM::BaseClass *item, bool show_def = false);
  std::pair<int, int> ScrollArea() override;
  std::optional<const UHDM::BaseClass *> ItemForHierarchy();

 private:
  // Variant that includes a flag indicating if the item change should save the
  // current state (it not empty). This is used for forward/back stack
  // navigation. If the state is saved, anything beyond current stack pointer
  // is wiped out.
  void SetItem(const UHDM::BaseClass *item, bool show_def, bool save_state);
  // Generates a nice header that probably fits in the given width.
  std::string GetHeader(int max_w);

  // Show values of highlighted items or not.
  bool show_vals_ = true;
  // Current column location of the cursor, with 0 being the start of the source
  // code line.
  int col_idx_ = 0;
  int max_col_idx_ = 0;
  int scroll_col_ = 0;
  // The instance or generate block whose source is shown.
  const UHDM::BaseClass *item_ = nullptr;
  bool showing_def_;
  // The currently selected item. Could be a parameter too.
  const UHDM::BaseClass *sel_ = nullptr;
  std::string sel_param_;
  // All source lines of the file containing the module instance containing the
  // selected item (this could be the complete instance itself too).
  std::string current_file_;
  std::vector<std::string> lines_;
  // Things in the source code that are navigable:
  // - Nets and variables. Add to wave, trace drivers/loads, go to def.
  // - Module instances: Open source
  // - Parameters: go to definition
  // These are stored in a hash by identifier so they can be appropriately
  // syntax-highlighted. Also stored by line to allow for fast lookup under the
  // cursor.
  std::unordered_map<std::string, const UHDM::BaseClass *> nav_;
  std::unordered_map<int, std::vector<std::pair<int, const UHDM::BaseClass *>>>
      nav_by_line_;
  // Map of all textual parameters and their definitions.
  std::unordered_map<std::string, std::string> params_;
  std::unordered_map<int, std::vector<std::pair<int, std::string>>>
      params_by_line_;
  // Tokenizer that holds identifiers and keywords for each line. This
  // simplifies syntax highlighting for keywords and comments.
  SimpleTokenizer tokenizer_;
  // Limits active scope, for example files with more than one module
  // definition. Text rendering uses this grey out source outside this.
  int start_line_ = 0;
  int end_line_ = 0;
  // When not null, indicates this object should be shown in the hierarchy. This
  // allows the hierarchy panel to always match current scope.
  const UHDM::BaseClass *item_for_hier_ = nullptr;

  // Stack of states, to allow going back/forth while browsing source.
  // TODO: Complete this and use it.
  struct State {
    const UHDM::BaseClass *item = nullptr;
    int line_idx = 0;
    int scroll_row = 0;
    bool show_def = false;
  };
  std::deque<State> state_stack_;
  int stack_idx_ = 0;
};

} // namespace sv
#endif // _SRC_SOURCE_H_
