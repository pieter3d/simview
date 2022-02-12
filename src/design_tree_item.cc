#include "design_tree_item.h"
#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "utils.h"
#include "workspace.h"
#include <uhdm/gen_scope.h>
#include <uhdm/gen_scope_array.h>
#include <uhdm/module.h>
#include <uhdm/task_func.h>

namespace sv {

DesignTreeItem::DesignTreeItem(UHDM::any *item) : item_(item) {
  name_ = StripWorklib(item_->VpiName());
  if (item_->VpiType() == vpiGenScopeArray) {
    type_ = "[generate]";
  } else if (item_->VpiType() == vpiFunction) {
    type_ = "[function]";
  } else if (item_->VpiType() == vpiTask) {
    type_ = "[task]";
  } else if (item_->VpiType() != vpiModule) {
    type_ = absl::StrFormat("[%d]", item_->VpiType());
  } else {
    if (ErrType()) {
      type_ = "[missing def]";
    } else {
      type_ = StripWorklib(item_->VpiDefName());
    }
  }
}

bool DesignTreeItem::AltType() const { return item_->VpiType() != vpiModule; }

bool DesignTreeItem::ErrType() const {
  // If there's no compiled variant available, Surelog uses :: to place it in
  // the scope of the parent module.
  return absl::StrContains(item_->VpiDefName(), "::");
}

bool DesignTreeItem::Expandable() const {
  if (!children_built_) BuildChildren();
  return !children_.empty();
}

int DesignTreeItem::NumChildren() const {
  if (!children_built_) BuildChildren();
  return children_.size();
}

TreeItem *DesignTreeItem::Child(int idx) {
  if (!children_built_) BuildChildren();
  return &children_[idx];
}

void DesignTreeItem::BuildChildren() const {
  if (item_->VpiType() == vpiModule) {
    auto m = dynamic_cast<const UHDM::module *>(item_);
    if (m->Modules() != nullptr) {
      for (auto &sub : *m->Modules()) {
        children_.push_back(DesignTreeItem(sub));
      }
    }
    if (m->Gen_scope_arrays() != nullptr) {
      for (auto &sub : *m->Gen_scope_arrays()) {
        children_.push_back(DesignTreeItem(sub));
      }
    }
    // TODO: These are definitions, not the task/function calls.
    if (m->Task_funcs() != nullptr) {
      for (auto &sub : *m->Task_funcs()) {
        children_.push_back(DesignTreeItem(sub));
      }
    }
    // TODO: Other stuff ?
  } else if (item_->VpiType() == vpiGenScopeArray) {
    // Working on the assumption that Surelog always just has one GenScope under
    // any GenScopeArray, since it always unrolls any loop. Even generate
    // if-statements get a wrapping GenScopeArray.
    auto ga = dynamic_cast<const UHDM::gen_scope_array *>(item_);
    auto g = (*ga->Gen_scopes())[0];
    if (g->Modules() != nullptr) {
      for (auto &sub : *g->Modules()) {
        children_.push_back(DesignTreeItem(sub));
      }
    }
    if (g->Gen_scope_arrays() != nullptr) {
      for (auto &sub : *g->Gen_scope_arrays()) {
        children_.push_back(DesignTreeItem(sub));
      }
    }
  }
  // TODO: Also offer lexical sort?
  std::stable_sort(children_.begin(), children_.end(),
                   [](const DesignTreeItem &a, const DesignTreeItem &b) {
                     return a.DesignItem()->VpiLineNo() <
                            b.DesignItem()->VpiLineNo();
                   });
  children_built_ = true;
}

bool DesignTreeItem::MatchColor() const {
  return item_ == Workspace::Get().MatchedDesignScope();
}

} // namespace sv
