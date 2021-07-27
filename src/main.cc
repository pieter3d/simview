#include "ErrorReporting/ErrorDefinition.h"
#include "ui.h"
#include <surelog/surelog.h>
#include <vector>

int main(int argc, const char *argv[]) {
  // Parse the design using the remaining command line arguments
  SURELOG::SymbolTable *symbolTable = new SURELOG::SymbolTable();
  SURELOG::ErrorContainer *errors = new SURELOG::ErrorContainer(symbolTable);
  SURELOG::CommandLineParser *clp =
      new SURELOG::CommandLineParser(errors, symbolTable);
  clp->noPython();
  clp->setParse(true);
  clp->setElaborate(true);
  clp->setCompile(true);
  clp->setwritePpOutput(true);
  clp->setMuteStdout();
  bool success = clp->parseCommandLine(argc, argv);
  SURELOG::Design *design = nullptr;
  if (success && !clp->help()) {
    SURELOG::scompiler *compiler = SURELOG::start_compiler(clp);
    design = SURELOG::get_design(compiler);
    SURELOG::shutdown_compiler(compiler);
    for (auto &err : errors->getErrors()) {
      auto [msg, fatal, filtered] = errors->createErrorMessage(err);
      // Surelog seems to like including newlines in the error messages.
      msg.erase(std::find_if(msg.rbegin(), msg.rend(),
                             [](unsigned char ch) { return !std::isspace(ch); })
                    .base(),
                msg.end());
      // It's complicated to find an error's severity...
      auto map = SURELOG::ErrorDefinition::getErrorInfoMap();
      auto err_info_it = map.find(err.getType());
      if (err_info_it == map.end() ||
          err_info_it->second.m_severity !=
              SURELOG::ErrorDefinition::ErrorSeverity::NOTE) {
        // Skip notes, it's cluttery.
        std::cout << msg << std::endl;
      }
    }
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

  sv::UI ui;
  ui.SetDesign(design);
  ui.EventLoop();

  delete design;

  return 0;
}
