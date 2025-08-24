#include "slang_utils.h"
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

} // namespace sv
