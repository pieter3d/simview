#pragma once

#include "slang/ast/Scope.h"
#include "slang/ast/Symbol.h"

namespace sv {

bool SymbolHasSubs(const slang::ast::Symbol *s);

const slang::ast::Scope *GetScopeForUI(const slang::ast::Symbol *sym);
const slang::ast::InstanceBodySymbol *GetContainingInstance(const slang::ast::Symbol *sym);

// Symbols for which the trace drivers / loads operation makes sense.
bool IsTraceable(const slang::ast::Symbol *sym);

} // namespace sv
