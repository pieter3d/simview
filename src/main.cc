#include "ErrorReporting/ErrorDefinition.h"
#include "ui.h"
#include <memory>
#include <surelog/surelog.h>
#include <vector>

int main(int argc, const char *argv[]) {
  // Parse the design using the remaining command line arguments
  auto symbolTable = std::make_unique<SURELOG::SymbolTable>();
  auto errors = std::make_unique<SURELOG::ErrorContainer>((symbolTable.get()));
  auto clp = std::make_unique<SURELOG::CommandLineParser>(errors.get(),
                                                          symbolTable.get());
  SURELOG::scompiler *compiler = nullptr;
  clp->noPython();
  clp->setParse(true);
  clp->setElaborate(true);
  clp->setCompile(true);
  clp->setwritePpOutput(true);
  bool success = clp->parseCommandLine(argc, argv);
  SURELOG::Design *design = nullptr;
  if (!success || clp->help()) return -1;
  clp->setMuteStdout();
  compiler = SURELOG::start_compiler(clp.get());
  design = SURELOG::get_design(compiler);
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
  sv::UI ui;
  ui.SetDesign(design);
  ui.EventLoop();

  SURELOG::shutdown_compiler(compiler);
  delete design;

  return 0;
}
