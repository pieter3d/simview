#include "slang_utils.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"

namespace sv {

bool SymbolHasSubs(const slang::ast::Symbol *s) {
  if (s == nullptr) return false;
  if (const auto *inst = s->as_if<slang::ast::InstanceSymbol>()) {
    return !inst->body.membersOfType<slang::ast::InstanceSymbol>().empty() ||
           !inst->body.membersOfType<slang::ast::GenerateBlockSymbol>().empty() ||
           !inst->body.membersOfType<slang::ast::GenerateBlockArraySymbol>().empty();
  }
  return false;
}

const slang::ast::InstanceBodySymbol *GetScopeForUI(const slang::ast::Symbol *sym) {
  while (sym->kind != slang::ast::SymbolKind::InstanceBody) {
    // If at the top, stop here.
    if (sym->getParentScope()->asSymbol().kind == slang::ast::SymbolKind::Root) break;
    sym = &sym->getParentScope()->asSymbol();
  }
  return sym->as_if<slang::ast::InstanceBodySymbol>();
}

bool IsTraceable(const slang::ast::Symbol *sym) {
  const slang::ast::SymbolKind k = sym->kind;
  return k == slang::ast::SymbolKind::Net || k == slang::ast::SymbolKind::Variable;
}
} // namespace sv
