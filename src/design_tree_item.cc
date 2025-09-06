#include "design_tree_item.h"
#include "absl/strings/str_cat.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "workspace.h"

namespace sv {

DesignTreeItem::DesignTreeItem(const slang::ast::Symbol *item) : item_(item) {
  if (item_->kind == slang::ast::SymbolKind::GenerateBlock) {
    type_ = "[generate]";
    auto &gen = item->as<slang::ast::GenerateBlockSymbol>();
    // Check if this block is part of an array. If so, use the array name and suffix the index.
    if (gen.arrayIndex) {
      name_ = absl::StrFormat("%s[%d]", gen.getParentScope()->asSymbol().name,
                              *gen.arrayIndex->as<int>());
    }
  } else if (item_->kind == slang::ast::SymbolKind::UninstantiatedDef) {
    type_ = "[missing def]";
  } else if (const auto *inst = item_->as_if<slang::ast::InstanceSymbol>()) {
    type_ = inst->getDefinition().name;
    name_ = inst->getArrayName();
    slang::SmallVector<slang::ConstantRange, 8> inst_array_vector;
    inst->getArrayDimensions(inst_array_vector);
    for (int i = 0; i < inst->arrayPath.size(); ++i) {
      absl::StrAppend(&name_,
                      absl::StrFormat("[%d]", inst->arrayPath[i] + inst_array_vector[i].lower()));
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
  auto visitor = slang::ast::makeVisitor(
      [&](auto &visitor, const slang::ast::InstanceSymbol &inst) {
        children_.push_back(DesignTreeItem(&inst));
      },
      [&](auto &visitor, const slang::ast::GenerateBlockSymbol &gen) {
        if (gen.isUninstantiated) return;
        children_.push_back(DesignTreeItem(&gen));
      },
      [&](auto &visitor, const slang::ast::UninstantiatedDefSymbol &undef) {
        children_.push_back(DesignTreeItem(&undef));
      });
  if (const auto *inst = item_->as_if<slang::ast::InstanceSymbol>()) {
    inst->body.visit(visitor);
  } else if (const auto *gen = item_->as_if<slang::ast::GenerateBlockSymbol>()) {
    for (const slang::ast::Symbol &sym : gen->members()) {
      sym.visit(visitor);
    }
  }

  // TODO: Also offer lexical sort? Currently it's just ordered by elaboration order.
  children_built_ = true;
}

bool DesignTreeItem::MatchColor() const {
  if (Workspace::Get().MatchedDesignScope() == nullptr) return false;
  if (const auto *body = Workspace::Get()
                             .MatchedDesignScope()
                             ->asSymbol()
                             .as_if<slang::ast::InstanceBodySymbol>()) {
    return item_ == body->parentInstance;
  }
  return false;
}

} // namespace sv
