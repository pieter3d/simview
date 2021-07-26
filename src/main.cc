#include "ui.h"
#include <surelog/surelog.h>
#include <vector>

int main(int argc, const char *argv[]) {
  // Parse the design using the remaining command line arguments
  SURELOG::SymbolTable *symbolTable = new SURELOG::SymbolTable();
  SURELOG::ErrorContainer *errors = new SURELOG::ErrorContainer(symbolTable);
  SURELOG::CommandLineParser *clp =
      new SURELOG::CommandLineParser(errors, symbolTable, false, false);
  clp->noPython();
  clp->setParse(true);
  clp->setElaborate(true);
  clp->setCompile(true);
  clp->setwritePpOutput(true);
  bool success = clp->parseCommandLine(argc, argv);
  SURELOG::Design *design = nullptr;
  if (success && !clp->help()) {
    SURELOG::scompiler *compiler = SURELOG::start_compiler(clp);
    design = SURELOG::get_design(compiler);
    SURELOG::shutdown_compiler(compiler);
    errors->printStats(errors->getErrorStats());
    auto stats = errors->getErrorStats();
    if (design == nullptr || /* stats.nbError > 0 i ||*/ stats.nbFatal > 0 ||
        stats.nbSyntax > 0) {
      std::cout << "Unable to parse the design!" << std::endl;
      return -1;
    }
    if (design->getTopLevelModuleInstances().empty()) {
      std::cout << "No top level design found!" << std::endl;
      return -1;
    }
  }
  delete clp;
  delete symbolTable;
  delete errors;

  // TODO: remove print.
  printf("Design top: %s\n",
         design->getTopLevelModuleInstances()[0]->getModuleName().c_str());

  sv::UI ui;
  ui.SetDesign(design);
  ui.EventLoop();

  delete design;

  return 0;
}
