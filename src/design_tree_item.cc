#include "design_tree_item.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/ParameterSymbols.h"
#include "slang/text/SourceManager.h"
#include "workspace.h"

namespace sv {

DesignTreeItem::DesignTreeItem(const slang::ast::Symbol *item, int idx) : DesignTreeItem(item) {
  name_ = absl::StrFormat("%s[%d]", name_, idx);
  index_ = idx;
}

DesignTreeItem::DesignTreeItem(const slang::ast::Symbol *item) : item_(item) {
  if (item_->kind == slang::ast::SymbolKind::GenerateBlock) {
    type_ = "[generate]";
    // If the block is part of an array, then grab the array's name instead.
    if (item->getParentScope()->asSymbol().kind == slang::ast::SymbolKind::GenerateBlockArray) {
      name_ = item->getParentScope()->asSymbol().name;
    }
  } else if (item_->kind == slang::ast::SymbolKind::UninstantiatedDef) {
    type_ = "[missing def]";
  } else if (const auto *inst = item_->as_if<slang::ast::InstanceSymbol>()) {
    type_ = inst->getDefinition().name;
    // Check if this instance is part of an array. If so, the name should be taken from the array.
    if (const auto *arr =
            item_->getHierarchicalParent()->asSymbol().as_if<slang::ast::InstanceArraySymbol>()) {
      name_ = arr->name;
    }
  } else {
    type_ = "!!Unknown!!";
  }
}

bool DesignTreeItem::AltType() const { return item_->kind != slang::ast::SymbolKind::Instance; }

bool DesignTreeItem::ErrType() const {
  return item_->kind == slang::ast::SymbolKind::UninstantiatedDef;
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
  const slang::ast::Scope *s = nullptr;
  if (const auto *inst = item_->as_if<slang::ast::InstanceSymbol>()) {
    s = &inst->body;
  } else if (const slang::ast::GenerateBlockSymbol *gen =
                 item_->as_if<slang::ast::GenerateBlockSymbol>()) {
    s = gen;
  }
  if (s == nullptr) return;
  for (const auto &inst : s->membersOfType<slang::ast::InstanceSymbol>()) {
    children_.push_back(DesignTreeItem(&inst));
  }
  for (const auto &arr : s->membersOfType<slang::ast::InstanceArraySymbol>()) {
    int idx = arr.range.left;
    const int end = arr.range.right;
    const bool increment = end > idx;
    for (const auto &inst : arr.membersOfType<slang::ast::InstanceSymbol>()) {
      children_.push_back(DesignTreeItem(&inst, idx));
      idx += increment ? 1 : -1;
    }
  }
  for (const auto &undef : s->membersOfType<slang::ast::UninstantiatedDefSymbol>()) {
    children_.push_back(DesignTreeItem(&undef));
  }
  for (const auto &arr : s->membersOfType<slang::ast::GenerateBlockArraySymbol>()) {
    int idx = 0;
    for (const auto &gen : arr.membersOfType<slang::ast::GenerateBlockSymbol>()) {
      // To find the index of this generate block, look for a parameter without an initializer.
      const slang::ast::ParameterSymbol *param = nullptr;
      for (const auto &p : gen.membersOfType<slang::ast::ParameterSymbol>()) {
        if (p.getInitializer() == nullptr) {
          param = &p;
          break;
        }
      }
      if (std::optional<int> genvar_val =
              (param ? param->getValue().integer().as<int>() : std::nullopt)) {
        children_.push_back(DesignTreeItem(&gen, *genvar_val));
      } else {
        children_.push_back(DesignTreeItem(&gen, idx));
      }
      idx++;
    }
  }
  for (const auto &gen : s->membersOfType<slang::ast::GenerateBlockSymbol>()) {
    if (gen.isUninstantiated) continue;
    children_.push_back(DesignTreeItem(&gen));
  }

  // Sort by, in order:
  // - Line number
  // - index (array instance index or looped generate index)
  // - construction index
  // TODO: Also offer lexical sort?
  const slang::SourceManager *sm = Workspace::Get().SourceManager();
  std::stable_sort(children_.begin(), children_.end(),
                   [&](const DesignTreeItem &a, const DesignTreeItem &b) {
                     const slang::ast::Symbol *sym_a = a.DesignItem();
                     const slang::ast::Symbol *sym_b = b.DesignItem();
                     const int line_a = sm->getLineNumber(sym_a->location);
                     const int line_b = sm->getLineNumber(sym_b->location);
                     if (line_a == line_b) {
                       if (a.index_ == b.index_) {
                         return a.DesignItem()->getIndex() < b.DesignItem()->getIndex();
                       } else {
                         return a.index_ < b.index_;
                       }
                     } else {
                       return line_a < line_b;
                     }
                   });
  children_built_ = true;
}

bool DesignTreeItem::MatchColor() const { return item_ == Workspace::Get().MatchedDesignScope(); }

} // namespace sv
