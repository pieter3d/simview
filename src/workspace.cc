#include "workspace.h"
#include <stdexcept>
#include <surelog/surelog.h>
#include <uhdm/headers/ElaboratorListener.h>
#include <uhdm/headers/module.h>
#include <uhdm/headers/vpi_uhdm.h>

namespace sv {

const UHDM::module *Workspace::GetDefinition(const UHDM::module *m) {
  // Top modules don't have a separate definition.
  if (m->VpiTopModule()) return m;
  const std::string &def_name = m->VpiDefName();
  if (module_defs_.find(def_name) == module_defs_.end()) {
    // Find the module definition in the UHDB.
    for (auto &candidate_module : *design_->AllModules()) {
      if (def_name == candidate_module->VpiDefName()) {
        module_defs_[def_name] = candidate_module;
        return candidate_module;
      }
    }
  }
  return module_defs_[def_name];
}

bool Workspace::ParseDesign(int argc, const char *argv[]) {
  // Parse the design using the remaining command line arguments
  SURELOG::ErrorContainer errors(&symbol_table_);
  SURELOG::CommandLineParser clp(&errors, &symbol_table_);
  clp.noPython();
  clp.setParse(true);
  clp.setElaborate(true);
  clp.setElabUhdm(true);
  clp.setCompile(true);
  clp.setwritePpOutput(true);
  clp.setWriteUhdm(false);
  bool success = clp.parseCommandLine(argc, argv);
  vpiHandle design = nullptr;
  if (!success || clp.help()) {
    if (!clp.help()) {
      std::cout << "Problems parsing argumnets." << std::endl;
    }
    return false;
  }
  clp.setMuteStdout();
  compiler_ = SURELOG::start_compiler(&clp);
  design = SURELOG::get_uhdm_design(compiler_);
  for (auto &err : errors.getErrors()) {
    auto [msg, fatal, filtered] = errors.createErrorMessage(err);
    // Surelog seems to like including newlines in the error messages.
    msg.erase(std::find_if(msg.rbegin(), msg.rend(),
                           [](unsigned char ch) { return !std::isspace(ch); })
                  .base(),
              msg.end());
    // It's complicated to find an error's severity...
    auto map = SURELOG::ErrorDefinition::getErrorInfoMap();
    auto err_info_it = map.find(err.getType());
    // Skip notes, it's cluttery.
    if (err_info_it == map.end() ||
        err_info_it->second.m_severity !=
            SURELOG::ErrorDefinition::ErrorSeverity::NOTE) {
      std::cout << msg << std::endl;
    }
  }
  auto stats = errors.getErrorStats();
  if (design == nullptr || /* stats.nbError > 0 i ||*/ stats.nbFatal > 0 ||
      stats.nbSyntax > 0) {
    std::cout << "Unable to parse the design!" << std::endl;
    return false;
  }
  // Pretty ugly cast here, both reinterpret and const...
  design_ = (UHDM::design *)((uhdm_handle *)design)->object;

  if (design_->TopModules()->empty()) {
    std::cout << "No top level design found!" << std::endl;
    return false;
  }

  for (auto id : clp.getIncludePaths()) {
    AddIncludeDir(symbol_table_.getSymbol(id));
  }

  return true;
}

bool Workspace::ReadWaves(const std::string &wave_file) {
  try {
    wave_data_ = WaveData::ReadWaveFile(wave_file);
  } catch (std::runtime_error &e) {
    return false;
  }
  return true;
}

Workspace::~Workspace() {
  if (compiler_ != nullptr) SURELOG::shutdown_compiler(compiler_);
  if (design_ != nullptr) delete design_;
}

void Workspace ::TryMatchDesignWithWaves() {
  // Don't do anything unless there are both waves and design.
  if (wave_data_ == nullptr || design_ == nullptr) return;
  // Look for a scope with stuff in it.
  std::vector<const WaveData::SignalScope *> signal_scopes;
  signal_scopes.push_back(&wave_data_->Root());
  for (const auto &sub : wave_data_->Root().children) {
    signal_scopes.push_back(&sub);
    for (const auto &subsub : sub.children) {
      signal_scopes.push_back(&subsub);
    }
  }
  // Okay, good enough. Now try to build a list of design scopes that look
  // interesting.
  std::vector<const UHDM::any *> design_scopes;
  for (const auto *sub : *design_->TopModules()) {
    design_scopes.push_back(sub);
    // If the top module has some guts, add it's modules too.
    if (sub->Modules() != nullptr && sub->Modules()->size() > 0) {
      for (const auto *subsub : *sub->Modules()) {
        design_scopes.push_back(subsub);
      }
    }
  }
  // Try to match the two candidate scopes against each other by looking for
  // matching signals.
  int max_common = 0;
  for (const auto *design_scope : design_scopes) {
    for (const auto *signal_scope : signal_scopes) {
      if (signal_scope->signals.empty()) continue;
      const auto *m = dynamic_cast<const UHDM::module *>(design_scope);
      if (m->Nets() == nullptr || m->Nets()->empty()) continue;
      int num_common_signals = 0;
      for (const auto *n : *m->Nets()) {
        for (const auto &s : signal_scope->signals) {
          if (n->VpiName() == s.name) {
            num_common_signals++;
          }
        }
      }
      // Save the best result.
      if (num_common_signals > max_common) {
        max_common = num_common_signals;
        matched_design_scope_ = design_scope;
        matched_signal_scope_ = signal_scope;
      }
    }
  }
}

} // namespace sv
