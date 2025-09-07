#include "slang_utils.h"

#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Scope.h"
#include "slang/ast/SemanticFacts.h"
#include "slang/ast/Symbol.h"
#include "slang/ast/expressions/AssignmentExpressions.h"
#include "slang/ast/expressions/SelectExpressions.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/PortSymbols.h"

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

const slang::ast::Scope *GetScopeForUI(const slang::ast::Symbol *sym) {
  while (sym->kind != slang::ast::SymbolKind::Root &&
         sym->kind != slang::ast::SymbolKind::InstanceBody &&
         sym->kind != slang::ast::SymbolKind::GenerateBlock) {
    sym = &sym->getHierarchicalParent()->asSymbol();
  }
  return sym->as_if<slang::ast::Scope>();
}

const slang::ast::InstanceBodySymbol *GetContainingInstance(const slang::ast::Symbol *sym) {
  while (sym != nullptr && sym->kind != slang::ast::SymbolKind::InstanceBody) {
    sym = &sym->getHierarchicalParent()->asSymbol();
  }
  return sym->as_if<slang::ast::InstanceBodySymbol>();
}

bool IsTraceable(const slang::ast::Symbol *sym) {
  const slang::ast::SymbolKind k = sym->kind;
  return k == slang::ast::SymbolKind::Net || k == slang::ast::SymbolKind::Variable;
}

std::vector<SlangDriverOrLoad> GetDrivers(const slang::ast::Symbol *sym) {
  // Helper class to traverse the instance looking for drivers.
  class DriverFinder : public slang::ast::ASTVisitor<DriverFinder, /*VisitStatements*/ true,
                                                     /*VisitExpressions*/ true> {
   public:
    DriverFinder(const slang::ast::Symbol *sym, std::vector<SlangDriverOrLoad> &d)
        : sym_(sym), drivers_(d) {}
    // Don't traverse into sub-instances, but do inspect the expressions on instance ports.
    void handle(const slang::ast::InstanceSymbol &inst) {
      // Iterate through all inout/outout ports to find the connections.
      for (const slang::ast::PortConnection *conn : inst.getPortConnections()) {
        if (const auto *port = conn->port.as_if<slang::ast::PortSymbol>()) {
          if (port->direction != slang::ast::ArgumentDirection::In) {
            if (const slang::ast::Expression *expr = conn->getExpression()) {
              checking_driving_port_ = true;
              expr->visit(*this);
              checking_driving_port_ = false;
            }
          }
        }
      }
    }
    // The module's input or inout ports could be a driver too.
    void handle(const slang::ast::PortSymbol &port) {
      if (port.direction != slang::ast::ArgumentDirection::Out && port.internalSymbol != nullptr &&
          port.internalSymbol == sym_) {
        drivers_.push_back(&port);
      }
    }
    // Assignments: only check the left side. Note that this also covers task/function call output
    // arguments.
    void handle(const slang::ast::AssignmentExpression &assignment) {
      checking_lhs_ = true;
      assignment.left().visit(*this);
      checking_lhs_ = false;
    }
    // For selection expressions, only check the value, not the selector.
    void handle(const slang::ast::RangeSelectExpression &expr) { expr.value().visit(*this); }
    void handle(const slang::ast::ElementSelectExpression &expr) { expr.value().visit(*this); }
    // Under the right conditions, named-value expressions matching the symbol are saved as drivers.
    void handle(const slang::ast::NamedValueExpression &nve) {
      if (&nve.symbol != sym_) return;
      if (checking_driving_port_ || checking_lhs_) {
        drivers_.push_back(&nve);
      }
    }
    // Skip instances without a definition, since the port directions are unknown.
    void handle(const slang::ast::UninstantiatedDefSymbol &uninst) {}

   private:
    const slang::ast::Symbol *sym_;
    std::vector<SlangDriverOrLoad> &drivers_;
    bool checking_driving_port_ = false;
    bool checking_lhs_ = false;
  };

  std::vector<SlangDriverOrLoad> drivers;
  // Look only in the containing instance.
  const slang::ast::InstanceBodySymbol *body = GetContainingInstance(sym);
  if (body == nullptr) return drivers;
  DriverFinder df(sym, drivers);
  body->visit(df);

  return drivers;
}

std::vector<SlangDriverOrLoad> GetLoads(const slang::ast::Symbol *sym) {
  std::vector<SlangDriverOrLoad> loads;
  return loads;
}

} // namespace sv
