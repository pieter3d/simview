#pragma once

#include "absl/container/btree_set.h"
#include "absl/container/flat_hash_map.h"
#include "panel.h"
#include "slang/ast/Expression.h"
#include "slang/ast/Symbol.h"
#include "source_buffer.h"
#include "src/slang_utils.h"

#include <deque>
#include <vector>

namespace sv {

// This panel displays source code for a SystemVerilog symbol.
class SourcePanel : public Panel {
 public:
  void Draw() final;
  void UIChar(int ch) final;
  int NumLines() const final { return src_.NumLines(); }
  std::optional<std::pair<int, int>> CursorLocation() const final;
  std::vector<Tooltip> Tooltips() const final;
  void SetItem(const slang::ast::Symbol *item);
  std::pair<int, int> ScrollArea() const final;
  std::optional<const slang::ast::Symbol *> ItemForDesignTree();
  std::optional<const slang::ast::Symbol *> ItemForWaves();
  bool Search(bool search_down) final;
  bool Searchable() const final { return true; }
  // Need to look for stuff under the cursor when changing lines.
  void SetLineAndScroll(int l) final;
  void Resized() final;
  void PrepareForDesignReload() final;
  void HandleReloadedDesign() final;

 private:
  void SaveState();
  // Variant that includes a flag indicating if the item change should save the
  // current state (if not empty). This is used for forward/back stack
  // navigation.
  void SetItem(const slang::ast::Symbol *item, bool save_state);
  // Jump to particular location in the current file, using the item's line and
  // column number.
  void SetLocation(const slang::ast::Symbol *sym);
  void SetLocation(const slang::ast::Expression *expr);
  void SetLocation(int line, int col);
  // Sets the selected item based on the cursor location
  void SelectItem();
  // Generates a nice header that probably fits in the current window width.
  void BuildHeader();
  // Read all wave data for the nets in the current item.
  void UpdateWaveData();

  // Textual representation of the current item. For things like nets the
  // containing scope is used.
  std::string header_;
  // Show values of highlighted items or not.
  bool show_vals_ = true;
  // State for tracking horizontal scrolling.
  // TODO: See what can be moved to the Panel class.
  int max_col_idx_ = 0;
  int scroll_col_ = 0;
  // The containing scope of the selection. Either instance body or generate block.
  const slang::ast::Scope *scope_ = nullptr;
  // The currently selected item. Could be a parameter too.
  const slang::ast::Symbol *sel_ = nullptr;
  // All source lines of the file containing the module instance containing the
  // selected item (this could be the complete instance itself too).
  std::string current_file_;
  SourceBuffer src_;
  // Store all relevant sections of each line for syntax highlighting and design tracing.
  struct SourceInfo {
    size_t start_col, end_col;
    const slang::ast::Symbol *sym = nullptr;
    const bool keyword = false;
    const bool comment = false;
    bool operator<(const SourceInfo &rhs) const { return start_col < rhs.start_col; }
  };
  absl::flat_hash_map<int, absl::btree_set<SourceInfo>> src_info_;
  void FindSymbols();
  void FindKeywordsAndComments();
  // Limits active scope, for example files with more than one module
  // definition. Text rendering uses this grey out source outside this.
  int start_line_ = 0;
  int end_line_ = 0;
  // When not null, indicates this object should be shown in the design tree.
  // This allows the design tree panel to always match current scope.
  const slang::ast::Symbol *item_for_design_tree_ = nullptr;
  const slang::ast::Symbol *item_for_waves_ = nullptr;
  // Drivers and loads
  int trace_idx_;
  std::vector<SlangDriverOrLoad> drivers_or_loads_;

  // Stack of states, to allow going back/forth while browsing source.
  struct State {
    const slang::ast::Scope *scope = nullptr;
    int line_idx = 0;
    int col_idx = 0;
    int scroll_row = 0;
    int scroll_col = 0;
  };
  std::deque<State> state_stack_;
  int stack_idx_ = 0;
  // When reloading the design, save the current scope as a textual path, so it can be re-found
  // after the reload has finished.
  std::string reload_path_;
  int reload_line_idx_ = 0;
};

} // namespace sv
