#include "design_tree_panel.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang_utils.h"
#include "workspace.h"
#include <memory>

namespace sv {

DesignTreePanel::DesignTreePanel() {
  // Populate an intial list of instances, top modules really.
  for (const slang::ast::InstanceSymbol *top : Workspace::Get().Design()->topInstances) {
    roots_.push_back(std::make_unique<DesignTreeItem>(top));
  }
  // Put all top modules with sub instances on top.
  // Lexical sort within that.
  std::stable_sort(
      roots_.begin(), roots_.end(),
      [](const std::unique_ptr<DesignTreeItem> &a, const std::unique_ptr<DesignTreeItem> &b) {
        const bool a_has_subs = SymbolHasSubs(a->DesignItem());
        const bool b_has_subs = SymbolHasSubs(b->DesignItem());
        if (a_has_subs == b_has_subs) {
          return a->DesignItem()->name < b->DesignItem()->name;
        } else {
          return a_has_subs;
        }
      });

  // Insert all roots into the tree data structure.
  for (const auto &r : roots_) {
    data_.AddRoot(r.get());
  }

  // First instance is pre-expanded for usability if there is just one.
  if (data_.ListSize() == 1) data_.ToggleExpand(0);
}

void DesignTreePanel::SetItem(const slang::ast::Symbol *item) {
  // First, build a list of all things up to the root.
  std::vector<const slang::ast::Symbol *> path;
  while (item != nullptr) {
    // Don't include the array wrapper since SystemVerilog hierarchical paths unroll the arrays.
    if (item->kind != slang::ast::SymbolKind::GenerateBlockArray) {
      path.push_back(item);
    }
    const slang::ast::Scope *parent_scope = item->getParentScope();
    if (parent_scope) {
      item = &parent_scope->asSymbol();
    } else {
      item = nullptr;
    }
  }
  // Now look through the list at every level and expand as necessary.
  // Iterate backwards so that the root is the first thing looked for.
  for (int path_idx = path.size() - 1; path_idx >= 0; --path_idx) {
    auto &path_item = path[path_idx];
    for (int item_idx = 0; item_idx < data_.ListSize(); ++item_idx) {
      const auto *list_item = dynamic_cast<const DesignTreeItem *>(data_[item_idx]);
      if (path_item == list_item->DesignItem()) {
        line_idx_ = item_idx;
        // Expand the item if it isn't already, and if it isn't the last one.
        if (list_item->Expandable() && !list_item->Expanded() && path_idx != 0) {
          data_.ToggleExpand(item_idx);
        }
        break;
      }
    }
  }
  SetLineAndScroll(line_idx_);
}

void DesignTreePanel::UIChar(int ch) {
  switch (ch) {
  case 'i': load_instance_ = true; break;
  case 'd': {
    // TODO: implement when source/waves are slang-ified.
    //// Make sure the definition exits.
    // auto *item =
    //     dynamic_cast<const DesignTreeItem *>(data_[line_idx_])->DesignItem();
    // if (item->VpiType() == vpiModule) {
    //   auto *m = dynamic_cast<const UHDM::module_inst *>(item);
    //   if (Workspace::Get().GetDefinition(m) == nullptr) {
    //     // If not, do nothing.
    //     error_message_ = absl::StrFormat("Definition of %s is not available.",
    //                                      StripWorklib(m->VpiFullName()));
    //     break;
    //   }
    // }
    // load_definition_ = true;
  } break;
  case 'S':
    // if (Workspace::Get().Waves() != nullptr) {
    //   Workspace::Get().SetMatchedDesignScope(
    //       dynamic_cast<const DesignTreeItem *>(data_[line_idx_])->DesignItem());
    // }
    break;
  default: TreePanel::UIChar(ch);
  }
}

std::optional<std::pair<const UHDM::any *, bool>> DesignTreePanel::ItemForSource() {
  // TODO: slang-ify this.
  // if (load_definition_ || load_instance_) {
  //  std::pair<const UHDM::any *, bool> ret = {
  //      dynamic_cast<const DesignTreeItem *>(data_[line_idx_])->DesignItem(),
  //      load_definition_};
  //  load_definition_ = false;
  //  load_instance_ = false;
  //  return ret;
  //}
  return std::nullopt;
}

std::vector<Tooltip> DesignTreePanel::Tooltips() const {
  std::vector<Tooltip> tooltips{
      {"i", "instance"},
      {"d", "definition"},
      {"u", "up scope"},
  };
  if (Workspace::Get().Waves() != nullptr) {
    tooltips.push_back({"S", "set scope for waves"});
  }
  return tooltips;
}

} // namespace sv
