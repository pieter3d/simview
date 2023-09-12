#include "design_tree_panel.h"
#include "absl/strings/str_format.h"
#include "utils.h"
#include "workspace.h"
#include <memory>
#include <uhdm/module_inst.h>

namespace sv {

DesignTreePanel::DesignTreePanel() {
  // Populate an intial list of instances, top modules really.
  for (auto &top : *Workspace::Get().Design()->TopModules()) {
    roots_.push_back(std::make_unique<DesignTreeItem>(top));
  }
  // Put all top modules with sub instances on top.
  // Lexical sort within that.
  std::stable_sort(
      roots_.begin(), roots_.end(),
      [](const std::unique_ptr<DesignTreeItem> &a,
         const std::unique_ptr<DesignTreeItem> &b) {
        auto ma = dynamic_cast<const UHDM::module_inst *>(a->DesignItem());
        auto mb = dynamic_cast<const UHDM::module_inst *>(b->DesignItem());
        bool a_has_subs = ma->Modules() != nullptr || ma->Gen_scope_arrays();
        bool b_has_subs = mb->Modules() != nullptr || mb->Gen_scope_arrays();
        if (a_has_subs == b_has_subs) {
          return ma->VpiName() < mb->VpiName();
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

void DesignTreePanel::SetItem(const UHDM::any *item) {
  // First, build a list of all things up to the root.
  std::vector<const UHDM::any *> path;
  while (item != nullptr) {
    // Skip GenScope, here we only consider GenScopeArray
    if (item->VpiType() != vpiGenScope) {
      path.push_back(item);
    }
    item = item->VpiParent();
  }
  // Now look through the list at every level and expand as necessary.
  // Iterate backwards so that the root is the first thing looked for.
  for (int path_idx = path.size() - 1; path_idx >= 0; --path_idx) {
    auto &path_item = path[path_idx];
    for (int item_idx = 0; item_idx < data_.ListSize(); ++item_idx) {
      const auto *list_item =
          dynamic_cast<const DesignTreeItem *>(data_[item_idx]);
      if (path_item == list_item->DesignItem()) {
        line_idx_ = item_idx;
        // Expand the item if it isn't already, and if it isn't the last one.
        if (list_item->Expandable() && !list_item->Expanded() &&
            path_idx != 0) {
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
    // Make sure the definition exits.
    auto *item =
        dynamic_cast<const DesignTreeItem *>(data_[line_idx_])->DesignItem();
    if (item->VpiType() == vpiModule) {
      auto *m = dynamic_cast<const UHDM::module_inst *>(item);
      if (Workspace::Get().GetDefinition(m) == nullptr) {
        // If not, do nothing.
        error_message_ = absl::StrFormat("Definition of %s is not available.",
                                         StripWorklib(m->VpiFullName()));
        break;
      }
    }
    load_definition_ = true;
  } break;
  case 'S':
    if (Workspace::Get().Waves() != nullptr) {
      Workspace::Get().SetMatchedDesignScope(
          dynamic_cast<const DesignTreeItem *>(data_[line_idx_])->DesignItem());
    }
    break;
  default: TreePanel::UIChar(ch);
  }
}

std::optional<std::pair<const UHDM::any *, bool>>
DesignTreePanel::ItemForSource() {
  if (load_definition_ || load_instance_) {
    std::pair<const UHDM::any *, bool> ret = {
        dynamic_cast<const DesignTreeItem *>(data_[line_idx_])->DesignItem(),
        load_definition_};
    load_definition_ = false;
    load_instance_ = false;
    return ret;
  }
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
