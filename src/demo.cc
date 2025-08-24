#include "external/abseil-cpp/absl/strings/str_format.h"
#include "external/slang/include/slang/driver/Driver.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include <iostream>

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
  std::cout << absl::StrFormat("root=%s\n", slang::ast::toString(root->kind));

  for (const slang::ast::InstanceSymbol *inst : root->topInstances) {
    std::cout << absl::StrFormat("top:%s empty=%d\n", inst->name, inst->body.empty());
  }

  return 0;
}
