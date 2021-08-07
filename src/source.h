#ifndef _SRC_SOURCE_H_
#define _SRC_SOURCE_H_

#include "panel.h"
#include "simple_tokenizer.h"
#include "workspace.h"
#include <uhdm/headers/BaseClass.h>
#include <uhdm/headers/module.h>
#include <vector>

namespace sv {

class Source : public Panel {
 public:
  Source(WINDOW *w, Workspace &ws) : Panel(w), workspace_(ws) {}
  void Draw() override;
  void UIChar(int ch) override;
  int NumLines() const override { return lines_.size(); }
  bool TransferPending() override;
  std::string Tooltip() const override;
  void SetItem(const UHDM::BaseClass *item, bool open_def = false);
  std::pair<int, int> ScrollArea() override;

 private:
  // Generates a nice header that probably fits in the given width.
  std::string GetHeader(int max_w);
  // Find the definition of the module that contains the given item.
  const UHDM::module *GetDefinition(const UHDM::module *m);

  // The selected instance or generate block.
  const UHDM::BaseClass *item_ = nullptr;
  bool showing_def_;
  // All source lines of the file containing the module instance containing the
  // selected item (this could be the complete instance itself too).
  std::string current_file_;
  std::vector<std::string> lines_;
  // Things in the source code that are navigable:
  // - Nets and variables. Add to wave, trace drivers/loads, go to def.
  // - Module instances: Open source
  // - Parameters: go to definition
  // These are stored in a hash by identifier so they can be appropriately
  // syntax-highlighted.
  std::unordered_map<std::string, const UHDM::BaseClass *> nav_;
  // Map of all textual parameters and their definitions.
  std::unordered_map<std::string, std::string> params_;
  // Reference to the workspace. Used to access the design database, waves, etc.
  Workspace &workspace_;
  // Tokenizer that holds identifiers and keywords for each line. This
  // simplifies syntax highlighting for keywords and comments.
  SimpleTokenizer tokenizer_;
  // Limits active scope, for example files with more than one module
  // definition. Text rendering uses this grey out source outside this.
  int start_line_ = 0;
  int end_line_ = 0;
  // Track all definitions of any given module instance. This not cleared
  // between item loads, but serves as a cache to avoid iterating over the
  // design's list of all module definitions.
  std::unordered_map<std::string, const UHDM::module *> module_defs_;

  // Stack of states, to allow going back/forth while browsing source.
  // TODO: Complete this and use it.
  struct State {
    UHDM::BaseClass *item = nullptr;
    int line_idx = 0;
  };
};

} // namespace sv
#endif // _SRC_SOURCE_H_
