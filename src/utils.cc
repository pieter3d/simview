#include "utils.h"
#include "event_control.h"
#include "module.h"
#include "ref_obj.h"
#include <uhdm/headers/assignment.h>
#include <uhdm/headers/begin.h>
#include <uhdm/headers/cont_assign.h>
#include <uhdm/headers/do_while.h>
#include <uhdm/headers/expr.h>
#include <uhdm/headers/for_stmt.h>
#include <uhdm/headers/if_else.h>
#include <uhdm/headers/if_stmt.h>
#include <uhdm/headers/named_begin.h>
#include <uhdm/headers/operation.h>
#include <uhdm/headers/part_select.h>
#include <uhdm/headers/process_stmt.h>
#include <uhdm/headers/ref_obj.h>
#include <uhdm/headers/task_call.h>
#include <uhdm/headers/tf_call.h>
#include <uhdm/headers/while_stmt.h>
#include <uhdm/include/sv_vpi_user.h>
#include <uhdm/include/vpi_user.h>

namespace sv {
namespace {

void RecurseFindItem(const UHDM::any *haystack, const UHDM::any *needle,
                     bool lhs, std::vector<const UHDM::any *> &list) {
  if (haystack == nullptr) return;
  auto type = haystack->VpiType();
  if (type == vpiOperation) {
    auto op = dynamic_cast<const UHDM::operation *>(haystack);
    // Recurse through the full expression.
    for (auto operand : *op->Operands()) {
      RecurseFindItem(operand, needle, lhs, list);
    }
  } else if (type == vpiBegin) {
    auto b = dynamic_cast<const UHDM::begin *>(haystack);
    if (b->Stmts() != nullptr) {
      for (auto s : *b->Stmts()) {
        RecurseFindItem(s, needle, lhs, list);
      }
    }
  } else if (type == vpiNamedBegin) {
    auto nb = dynamic_cast<const UHDM::named_begin *>(haystack);
    if (nb->Stmts() != nullptr) {
      for (auto s : *nb->Stmts()) {
        RecurseFindItem(s, needle, lhs, list);
      }
    }
  } else if (type == vpiFuncCall || type == vpiTaskCall) {
    auto tfc = dynamic_cast<const UHDM::tf_call *>(haystack);
    if (tfc->Tf_call_args() != nullptr) {
      for (auto a : *tfc->Tf_call_args()) {
        RecurseFindItem(a, needle, lhs, list);
      }
    }
  } else if (type == vpiAssignment) {
    auto assignment = dynamic_cast<const UHDM::assignment *>(haystack);
    auto expr = lhs ? assignment->Lhs() : assignment->Rhs();
    RecurseFindItem(expr, needle, lhs, list);
  } else if (type == vpiEventControl) {
    auto ec = dynamic_cast<const UHDM::event_control *>(haystack);
    RecurseFindItem(ec->Stmt(), needle, lhs, list);
  } else if (type == vpiIf) {
    auto is = dynamic_cast<const UHDM::if_stmt *>(haystack);
    RecurseFindItem(is->VpiStmt(), needle, lhs, list);
  } else if (type == vpiIfElse) {
    auto ie = dynamic_cast<const UHDM::if_else *>(haystack);
    RecurseFindItem(ie->VpiStmt(), needle, lhs, list);
    RecurseFindItem(ie->VpiElseStmt(), needle, lhs, list);
  } else if (type == vpiFor) {
    auto f = dynamic_cast<const UHDM::for_stmt *>(haystack);
    RecurseFindItem(f->VpiStmt(), needle, lhs, list);
  } else if (type == vpiWhile) {
    auto w = dynamic_cast<const UHDM::while_stmt *>(haystack);
    RecurseFindItem(w->VpiStmt(), needle, lhs, list);
  } else if (type == vpiDoWhile) {
    auto dw = dynamic_cast<const UHDM::do_while *>(haystack);
    RecurseFindItem(dw->VpiStmt(), needle, lhs, list);
  } else if (type == vpiPartSelect) {
    auto ps = dynamic_cast<const UHDM::part_select *>(haystack);
    if (ps->VpiParent() != nullptr && ps->VpiParent()->VpiType() == vpiRefObj) {
      auto ro = dynamic_cast<const UHDM::ref_obj *>(ps->VpiParent());
      if (ro->Actual_group() == needle) {
        list.push_back(haystack);
      }
    }
  } else if (type == vpiRefObj) {
    auto ro = dynamic_cast<const UHDM::ref_obj *>(haystack);
    if (ro->Actual_group() == needle) {
      list.push_back(haystack);
    }
  }
}

} // namespace

void GetDriversOrLoads(const UHDM::any *item, bool drivers,
                       std::vector<const UHDM::any *> &list) {
  list.clear();
  if (item == nullptr) return;
  if (!IsTraceable(item)) return;
  // First, find the containing module.
  auto m = GetContainingModule(item);
  // There should always be a containing module, but just in case:
  if (m == nullptr) return;
  // Look through all continuous assignments.
  if (m->Cont_assigns() != nullptr) {
    for (auto &ca : *m->Cont_assigns()) {
      // See if the net is part of the LHS.
      RecurseFindItem(drivers ? ca->Lhs() : ca->Rhs(), item, drivers, list);
    }
  }
  // Look through all process statements.
  if (m->Process() != nullptr) {
    for (auto &p : *m->Process()) {
      RecurseFindItem(p->Stmt(), item, drivers, list);
    }
  }
  if (!drivers) {
    for (auto x : list) {
      printf("%d:%d\r\n", x->VpiLineNo(), x->VpiType());
    }
  }
  // Check to see if the net is a module input or inout.
  // TODO
}

const UHDM::module *GetContainingModule(const UHDM::any *item) {
  do {
    item = item->VpiParent();
  } while (item != nullptr && item->VpiType() != vpiModule);
  return dynamic_cast<const UHDM::module *>(item);
}

std::string StripWorklib(const std::string &s) {
  const int lib_delimieter_pos = s.find('@');
  if (lib_delimieter_pos == std::string::npos) return s;
  return s.substr(lib_delimieter_pos + 1);
}

bool IsTraceable(const UHDM::any *item) {
  const int type = item->VpiType();
  return type == vpiNet || type == vpiLongIntVar || type == vpiShortIntVar ||
         type == vpiIntVar || type == vpiShortRealVar || type == vpiByteVar ||
         type == vpiClassVar || type == vpiStringVar || type == vpiEnumVar ||
         type == vpiStructVar || type == vpiUnionVar || type == vpiBitVar;
}

} // namespace sv
