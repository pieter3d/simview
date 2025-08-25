#include "external/abseil-cpp/absl/strings/str_format.h"
#include "external/slang/include/slang/driver/Driver.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/SystemSubroutine.h"
#include "slang/ast/expressions/CallExpression.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include <iostream>
#include <stack>

class NetVariableCollector : public slang::ast::ASTVisitor<NetVariableCollector, true, true> {
 private:
  std::stack<const slang::ast::Scope *> scope_stack_;

 public:
  template <typename T>
  void handle(const T &t) {
    if constexpr (std::is_base_of_v<slang::ast::Scope, T>) {
      scope_stack_.push(&t);
    }
    if constexpr (std::is_base_of_v<slang::ast::NetSymbol, T> ||
                  std::is_base_of_v<slang::ast::VariableSymbol, T>) {
      std::cout << absl::StrFormat("net:%s, in %s\n", t.name, t.getParentScope()->asSymbol().name);
    } else if constexpr (std::is_base_of_v<slang::ast::CallExpression, T>) {
      if (const slang::ast::SubroutineSymbol *const *subr =
              std::get_if<const slang::ast::SubroutineSymbol *>(&t.subroutine)) {
        std::cout << absl::StrFormat("call:%s, in %s\n", (*subr)->name,
                                     scope_stack_.top()->asSymbol().name);
      } else if (const slang::ast::CallExpression::SystemCallInfo *sysc =
                     std::get_if<slang::ast::CallExpression::SystemCallInfo>(&t.subroutine)) {
        std::cout << absl::StrFormat("syscall:%s, in %s\n", sysc->subroutine->name,
                                     scope_stack_.top()->asSymbol().name);
      }
    }
    if constexpr (!std::is_base_of_v<slang::ast::InstanceSymbol, T>) visitDefault(t);
    if constexpr (std::is_base_of_v<slang::ast::Scope, T>) {
      scope_stack_.pop();
    }
  }
};

int main(int argc, char *argv[]) {

  slang::driver::Driver driver;
  driver.addStandardArgs();
  if (!driver.parseCommandLine(argc, argv)) return -1;
  if (!driver.processOptions()) return -1;
  std::cout << "Parsing files...\n";
  if (!driver.parseAllSources()) return -1;
  std::unique_ptr<slang::ast::Compilation> compilation = driver.createCompilation();
  const slang::SourceManager *src = compilation->getSourceManager();
  (void)src;
  std::cout << "Elaborating...\n";
  const slang::ast::RootSymbol *root = &compilation->getRoot();
  std::cout << "compilation report...\n";
  driver.reportCompilation(*compilation, /* quiet */ false);
  std::cout << "compilation diags...\n";
  const bool success = driver.reportDiagnostics(/* quiet */ false);
  (void)success;

  for (const slang::ast::InstanceSymbol *inst : root->topInstances) {
    if (inst->getParentScope() == nullptr) {
      std::cout << absl::StrFormat("%s has null parent\n", inst->name);
    } else {
      std::cout << absl::StrFormat("%s parent: %s\n", inst->name, inst->getParentScope()->asSymbol().name);
    }
    if (inst->name == "top") {
      NetVariableCollector c;
      inst->body.visit(c);
      if (inst->body.getSyntax()) {
      printf("top is from %ld to %ld\n",
          src->getLineNumber(inst->body.getSyntax()->sourceRange().start()),
          src->getLineNumber(inst->body.getSyntax()->sourceRange().end()));
      } else {
        printf("syntax is null\n");
      }
    }
  }

  return 0;
}
