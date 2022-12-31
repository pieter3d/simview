#pragma once

#include "absl/container/flat_hash_map.h"
#include "panel.h"
#include "simple_tokenizer.h"
#include "workspace.h"
#include <deque>
#include <uhdm/uhdm_types.h>
#include <vector>

namespace sv {

// This panel displays source code for a UHDM item.
class SourcePanel : public Panel {
 public:
  void Draw() final;
  void UIChar(int ch) final;
  int NumLines() const final { return lines_.size(); }
  std::optional<std::pair<int, int>> CursorLocation() const final;
  std::vector<Tooltip> Tooltips() const final;
  void SetItem(const UHDM::any *item, bool show_def = false);
  std::pair<int, int> ScrollArea() const final;
  std::optional<const UHDM::any *> ItemForDesignTree();
  std::optional<const UHDM::any *> ItemForWaves();
  bool Search(bool search_down) final;
  bool Searchable() const final { return true; }
  // Need to look for stuff under the cursor when changing lines.
  void SetLineAndScroll(int l) final;
  void Resized() final;

 private:
  // Push the current state onto the stack.
  void SaveState();
  // Variant that includes a flag indicating if the item change should save the
  // current state (it not empty). This is used for forward/back stack
  // navigation.
  void SetItem(const UHDM::any *item, bool show_def, bool save_state);
  // Jump to particular location in the current file, using the item's line and
  // column number.
  void SetLocation(const UHDM::any *item);
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
  // Current column location of the cursor, with 0 being the start of the source
  // code line. TODO: See what can be moved to the Panel class.
  int max_col_idx_ = 0;
  int scroll_col_ = 0;
  // The instance or generate block whose source is shown.
  const UHDM::any *scope_ = nullptr;
  bool showing_def_;
  // The currently selected item. Could be a parameter too.
  const UHDM::any *sel_ = nullptr;
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
  absl::flat_hash_map<std::string, const UHDM::any *> nav_;
  absl::flat_hash_map<int, std::vector<std::pair<int, const UHDM::any *>>>
      nav_by_line_;
  // Map of all textual parameters and their definitions.
  absl::flat_hash_map<std::string, std::string> params_;
  absl::flat_hash_map<int, std::vector<std::pair<int, std::string>>>
      params_by_line_;
  // Tokenizer that holds identifiers and keywords for each line. This
  // simplifies syntax highlighting for keywords and comments.
  SimpleTokenizer tokenizer_;
  // Limits active scope, for example files with more than one module
  // definition. Text rendering uses this grey out source outside this.
  int start_line_ = 0;
  int end_line_ = 0;
  // When not null, indicates this object should be shown in the design tree.
  // This allows the design tree panel to always match current scope.
  const UHDM::any *item_for_design_tree_ = nullptr;
  const UHDM::any *item_for_waves_ = nullptr;
  // Drivers and loads
  const UHDM::any *trace_net_;
  int trace_idx_;
  bool trace_drivers_;
  std::vector<const UHDM::any *> drivers_or_loads_;

  // Stack of states, to allow going back/forth while browsing source.
  struct State {
    const UHDM::any *scope = nullptr;
    int line_idx = 0;
    int col_idx = 0;
    int scroll_row = 0;
    int scroll_col = 0;
    bool show_def = false;
  };
  std::deque<State> state_stack_;
  int stack_idx_ = 0;
};

} // namespace sv
