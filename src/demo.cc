#include "external/abseil-cpp/absl/strings/str_format.h"
#include "external/slang/include/slang/driver/Driver.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Expression.h"
#include "slang/ast/expressions/MiscExpressions.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/parsing/Token.h"
#include "slang/parsing/TokenKind.h"
#include "slang/syntax/SyntaxNode.h"
#include "slang/syntax/SyntaxVisitor.h"
#include <concepts>
#include <iostream>

int main(int argc, char *argv[]) {

  slang::driver::Driver driver;
  driver.addStandardArgs();
  if (!driver.parseCommandLine(argc, argv)) return -1;
  if (!driver.processOptions()) return -1;
  std::cout << "Parsing files...\n";
  if (!driver.parseAllSources()) return -1;
  std::unique_ptr<slang::ast::Compilation> compilation = driver.createCompilation();
  static const slang::SourceManager *src = compilation->getSourceManager();
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
      std::cout << absl::StrFormat("%s parent: %s\n", inst->name,
                                   inst->getParentScope()->asSymbol().name);
    }
    if (inst->name == "top") {
      //class TokenVisitor : public slang::syntax::SyntaxVisitor<TokenVisitor> {
      // public:
      //  void visitToken(slang::parsing::Token tk) {
      //    std::cout << absl::StrFormat("%s = %s (%d,%d)\n", slang::parsing::toString(tk.kind),
      //                                 tk.toString(), 
      //                                 src->getColumnNumber(tk.range().start()),
      //                                 src->getColumnNumber(tk.range().end())
      //                                 );
      //    for (auto &tr : tk.trivia()) {
      //      std::cout << absl::StrFormat("  T: %s = %s\n", slang::parsing::toString(tr.kind),
      //                                   tr.getRawText());
      //    }
      //  }
      //};
      //TokenVisitor tv;
      //inst->body.getSyntax()->visit(tv);
      //inst->visit(slang::ast::makeVisitor([&](const std::derived_from<slang::ast::Expression> auto &expr) {
      //      std::cout << absl::StrFormat("%s at %d,%d\n", slang::ast::toString(expr.kind), 
      //          src->getLineNumber(expr.sourceRange.start()),
      //          src->getLineNumber(expr.sourceRange.end()));
      //      }));
      class EV : public slang::ast::ASTVisitor<EV, true, true> {
       public:
        void handle(const slang::ast::InstanceSymbol &inst) { }
        void handle(const slang::ast::NamedValueExpression &expr) {
            std::cout << absl::StrFormat("%s at %d,%d-%d : %s\n", slang::ast::toString(expr.kind), 
                src->getLineNumber(expr.sourceRange.start()),
                src->getColumnNumber(expr.sourceRange.start()),
                src->getColumnNumber(expr.sourceRange.end()),
                expr.symbol.name);
        }
      };
      EV ev;
      inst->body.visit(ev);

    }
  }

  return 0;
}
