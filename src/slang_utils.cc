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

namespace {

// Helper class to traverse the instance looking for drivers or loads.
template <bool DRIVERS>
class DriverOrLoadFinder
    : public slang::ast::ASTVisitor<DriverOrLoadFinder<DRIVERS>, /*VisitStatements*/ true,
                                    /*VisitExpressions*/ true> {
 public:
  DriverOrLoadFinder(const slang::ast::Symbol *sym, std::vector<SlangDriverOrLoad> &d)
      : sym_(sym), drivers_or_loads_(d) {}
  // Don't traverse into sub-instances, but do inspect the expressions on instance ports.
  void handle(const slang::ast::InstanceSymbol &inst) {
    // Iterate through all ports to find the connections that are driving/loading.
    for (const slang::ast::PortConnection *conn : inst.getPortConnections()) {
      if (const auto *port = conn->port.as_if<slang::ast::PortSymbol>()) {
        if (port->direction !=
            (DRIVERS ? slang::ast::ArgumentDirection::In : slang::ast::ArgumentDirection::Out)) {
          if (const slang::ast::Expression *expr = conn->getExpression()) {
            checking_instance_port_expression_ = true;
            expr->visit(*this);
            checking_instance_port_expression_ = false;
          }
        }
      }
    }
  }
  // The module's ports could be a driver/load too.
  void handle(const slang::ast::PortSymbol &port) {
    if (port.direction !=
            (DRIVERS ? slang::ast::ArgumentDirection::Out : slang::ast::ArgumentDirection::In) &&
        port.internalSymbol != nullptr && port.internalSymbol == sym_) {
      drivers_or_loads_.push_back(&port);
    }
  }
  // Assignments: make note of which side is being checked. Note that function calls on the right
  // side that have output arguments result in further nested assignments with their own left/right.
  void handle(const slang::ast::AssignmentExpression &assignment) {
    checking_lhs_ = true;
    assignment.left().visit(*this);
    checking_lhs_ = false;
    checking_rhs_ = true;
    assignment.right().visit(*this);
    checking_rhs_ = false;
  }
  // For selection expressions, only check the value for drivers, not the selector. Both should be
  // checked in the case of loads.
  void handle(const slang::ast::RangeSelectExpression &expr) {
    expr.value().visit(*this);
    if constexpr (!DRIVERS) {
      selector_depth_++;
      expr.left().visit(*this);
      expr.right().visit(*this);
      selector_depth_--;
    }
  }
  void handle(const slang::ast::ElementSelectExpression &expr) {
    expr.value().visit(*this);
    if constexpr (!DRIVERS) {
      selector_depth_++;
      expr.selector().visit(*this);
      selector_depth_--;
    }
  }
  // Under the right conditions, named-value expressions matching the symbol are saved as drivers.
  void handle(const slang::ast::NamedValueExpression &nve) {
    if (&nve.symbol != sym_) return;
    if constexpr (DRIVERS) {
      if (checking_instance_port_expression_ || checking_lhs_) {
        drivers_or_loads_.push_back(&nve);
      }
    } else {
      // Any matching NamedValue is a load unless it's a left-side non-selector.
      if (!(checking_lhs_ && selector_depth_ == 0)) {
        drivers_or_loads_.push_back(&nve);
      }
    }
  }
  // Skip instances without a definition, since the port directions are unknown.
  void handle(const slang::ast::UninstantiatedDefSymbol &uninst) {}

 private:
  const slang::ast::Symbol *sym_;
  std::vector<SlangDriverOrLoad> &drivers_or_loads_;
  bool checking_instance_port_expression_ = false;
  bool checking_lhs_ = false;
  bool checking_rhs_ = false;
  int selector_depth_ = 0;
};

} // namespace

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
  std::vector<SlangDriverOrLoad> drivers;
  // Look only in the containing instance.
  const slang::ast::InstanceBodySymbol *body = GetContainingInstance(sym);
  if (body == nullptr) return drivers;
  DriverOrLoadFinder</*DRIVERS*/ true> df(sym, drivers);
  body->visit(df);
  return drivers;
}

std::vector<SlangDriverOrLoad> GetLoads(const slang::ast::Symbol *sym) {
  std::vector<SlangDriverOrLoad> loads;
  // Look only in the containing instance.
  const slang::ast::InstanceBodySymbol *body = GetContainingInstance(sym);
  if (body == nullptr) return loads;
  DriverOrLoadFinder</*DRIVERS*/ false> lf(sym, loads);
  body->visit(lf);
  return loads;
}

} // namespace sv
