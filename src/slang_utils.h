#pragma once

#include "slang/ast/Scope.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/symbols/PortSymbols.h"

namespace sv {

// Drivers and loads  are typically some named value expression in an assignment. But they could
// also be a module's input or output port. One is a Symbol and one an Expression, so a combined
// type is needed.
using SlangDriverOrLoad = std::variant<const slang::ast::PortSymbol*, const slang::ast::NamedValueExpression*>;

bool SymbolHasSubs(const slang::ast::Symbol *s);

const slang::ast::Scope *GetScopeForUI(const slang::ast::Symbol *sym);

const slang::ast::InstanceBodySymbol *GetContainingInstance(const slang::ast::Symbol *sym);

// Symbols for which the trace drivers / loads operation makes sense.
bool IsTraceable(const slang::ast::Symbol *sym);

std::vector<SlangDriverOrLoad> GetDrivers(const slang::ast::Symbol *sym);
std::vector<SlangDriverOrLoad> GetLoads(const slang::ast::Symbol *sym);

} // namespace sv
