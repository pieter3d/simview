#include "design_tree_item.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "workspace.h"

namespace sv {

DesignTreeItem::DesignTreeItem(const slang::ast::Symbol *item) : item_(item) {
  if (item_->kind == slang::ast::SymbolKind::GenerateBlock) {
    type_ = "[generate]";
    // If the block is part of an array, then grab the array's name and suffix the index.
    if (item->getParentScope()->asSymbol().kind == slang::ast::SymbolKind::GenerateBlockArray) {
      name_ = absl::StrFormat("%s[%d]", item->getParentScope()->asSymbol().name,
                              item_->as<slang::ast::GenerateBlockSymbol>().constructIndex);
    }
  } else if (item_->kind == slang::ast::SymbolKind::UninstantiatedDef) {
    type_ = "[missing def]";
  } else if (const slang::ast::InstanceSymbol *inst = item_->as_if<slang::ast::InstanceSymbol>()) {
    type_ = inst->getDefinition().name;
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
  if (const slang::ast::InstanceSymbol *inst = item_->as_if<slang::ast::InstanceSymbol>()) {
    s = &inst->body;
  } else if (const slang::ast::GenerateBlockSymbol *gen =
                 item_->as_if<slang::ast::GenerateBlockSymbol>()) {
    s = gen;
  }
  if (s == nullptr) return;
  for (const slang::ast::InstanceSymbol &inst : s->membersOfType<slang::ast::InstanceSymbol>()) {
    children_.push_back(DesignTreeItem(&inst));
  }
  for (const slang::ast::UninstantiatedDefSymbol &undef :
       s->membersOfType<slang::ast::UninstantiatedDefSymbol>()) {
    children_.push_back(DesignTreeItem(&undef));
  }
  for (const slang::ast::GenerateBlockArraySymbol &arr :
       s->membersOfType<slang::ast::GenerateBlockArraySymbol>()) {
    for (const slang::ast::GenerateBlockSymbol &gen :
         arr.membersOfType<slang::ast::GenerateBlockSymbol>()) {
      children_.push_back(DesignTreeItem(&gen));
    }
  }
  for (const slang::ast::GenerateBlockSymbol &gen :
       s->membersOfType<slang::ast::GenerateBlockSymbol>()) {
    if (gen.isUninstantiated) continue;
    children_.push_back(DesignTreeItem(&gen));
  }

  // TODO: Also offer lexical sort?
  std::stable_sort(children_.begin(), children_.end(),
                   [](const DesignTreeItem &a, const DesignTreeItem &b) {
                     return a.DesignItem()->getIndex() < b.DesignItem()->getIndex();
                   });
  children_built_ = true;
}

bool DesignTreeItem::MatchColor() const { return item_ == Workspace::Get().MatchedDesignScope(); }

} // namespace sv
