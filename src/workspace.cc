#include "workspace.h"
#include "uhdm_utils.h"
#include "utils.h"

#include <Surelog/Common/FileSystem.h>
#include <algorithm>
#include <iostream>
#include <stack>
#include <stdexcept>
#include <uhdm/ElaboratorListener.h>
#include <uhdm/array_net.h>
#include <uhdm/array_var.h>
#include <uhdm/gen_scope.h>
#include <uhdm/gen_scope_array.h>
#include <uhdm/module_inst.h>
#include <uhdm/net.h>
#include <uhdm/variables.h>
#include <uhdm/vpi_uhdm.h>

namespace sv {

namespace {

// Helper match, since the design names have a "work@" prefix.
bool ScopeMatch(std::string_view design_scope,
                std::string_view signal_scope) {
  auto pos = design_scope.find(signal_scope);
  if (pos == std::string::npos) return false;
  if (design_scope.size() == signal_scope.size() || pos == 0) return true;
  return design_scope[pos - 1] == '@';
};

} // namespace

const UHDM::module_inst *Workspace::GetDefinition(const UHDM::module_inst *m) {
  // Top modules don't have a separate definition.
  if (m->VpiTopModule()) return m;
  std::string_view def_name = m->VpiDefName();
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
  clp.setWriteUhdm(true);
  bool success = clp.parseCommandLine(argc, argv);
  vpiHandle design = nullptr;
  if (!success || clp.help()) {
    if (!clp.help()) {
      std::cout << "Problems parsing arguments." << std::endl;
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


  SURELOG::FileSystem *fs = SURELOG::FileSystem::getInstance();
  for (const auto &id : clp.getIncludePaths()) {
    AddIncludeDir(fs->toPath(id));
  }

  return true;
}

bool Workspace::ReadWaves(const std::string &wave_file) {
  try {
    wave_data_ = WaveData::ReadWaveFile(wave_file);
  } catch (std::runtime_error &e) {
    return false;
  }
  return wave_data_ != nullptr;
}

Workspace::~Workspace() {
  if (compiler_ != nullptr) SURELOG::shutdown_compiler(compiler_);
  delete design_;
}

void Workspace::TryMatchDesignWithWaves() {
  // Don't do anything unless there are both waves and design.
  if (wave_data_ == nullptr || design_ == nullptr) return;
  // Look for a scope with stuff in it.
  std::vector<const WaveData::SignalScope *> signal_scopes;
  for (const auto &root_scope : wave_data_->Roots()) {
    signal_scopes.push_back(&root_scope);
    for (const auto &sub : root_scope.children) {
      signal_scopes.push_back(&sub);
      for (const auto &subsub : sub.children) {
        signal_scopes.push_back(&subsub);
      }
    }
  }
  // Okay, good enough. Now try to build a list of design scopes that look
  // interesting. Not considering generate blocks here for the auto-detect.
  std::vector<const UHDM::any *> design_scopes;
  for (const auto *top : *design_->TopModules()) {
    design_scopes.push_back(top);
    // If the top module has some guts, add it's modules too.
    if (top->Modules() != nullptr && !top->Modules()->empty()) {
      for (const auto *sub : *top->Modules()) {
        design_scopes.push_back(sub);
        if (sub->Modules() != nullptr && !sub->Modules()->empty()) {
          for (const auto *subsub : *sub->Modules()) {
            design_scopes.push_back(subsub);
          }
        }
      }
    }
  }
  // Try to match the two candidate scopes against each other by looking for
  // matching signals.
  int max_common = 0;
  for (const auto *design_scope : design_scopes) {
    const auto *m = dynamic_cast<const UHDM::module_inst *>(design_scope);
    for (const auto *signal_scope : signal_scopes) {
      if (signal_scope->signals.empty()) continue;
      int num_common_signals = 0;
      if (m->Nets() != nullptr) {
        for (const auto *n : *m->Nets()) {
          for (const auto &s : signal_scope->signals) {
            if (n->VpiName() == s.name) {
              num_common_signals++;
            }
          }
        }
      }
      if (m->Variables() != nullptr) {
        for (const auto *v : *m->Variables()) {
          for (const auto &s : signal_scope->signals) {
            if (v->VpiName() == s.name) {
              num_common_signals++;
            }
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

std::vector<const WaveData::Signal *>
Workspace::DesignToSignals(const UHDM::any *item) const {
  std::vector<const WaveData::Signal *> signals;
  // Make sure it's actually something that would have ended up in a wave.
  if (!IsTraceable(item)) return signals;
  // Build a stack of all parents up to the top module, or the matched design
  // scope, whichever comes first.
  std::stack<const UHDM::any *> stack;
  const UHDM::any *parent = item->VpiParent();
  while (parent != nullptr) {
    // Drop out any genscopes, there's always a genscope_array above it.
    if (parent->VpiType() == vpiGenScope) parent = parent->VpiParent();
    stack.push(parent);
    if (parent == matched_design_scope_) break;
    parent = parent->VpiParent();
  }

  const WaveData::SignalScope *signal_scope = matched_signal_scope_;
  while (!stack.empty()) {
    auto *design_scope = stack.top();
    bool found = false;
    if (ScopeMatch(design_scope->VpiName(), signal_scope->name)) {
      found = true;
    } else {
      for (const auto &sub : signal_scope->children) {
        if (ScopeMatch(design_scope->VpiName(), sub.name)) {
          signal_scope = &sub;
          found = true;
          break;
        }
      }
    }
    // Total abort if there is no matching scope in the wave data.
    if (!found) return signals;
    stack.pop();
  }
  // Now add any signals in the drilled-down scope that match the net name
  // exactly, with optional square bracket suffixes.
  const std::string net_name = StripWorklib(item->VpiName());
  for (const auto &signal : signal_scope->signals) {
    auto pos = signal.name.find(StripWorklib(item->VpiName()));
    if (pos != 0) continue;
    if (signal.name.size() > net_name.size() &&
        signal.name[net_name.size()] != '[') {
      continue;
    }
    signals.push_back(&signal);
  }

  return signals;
}

const UHDM::any *
Workspace::SignalToDesign(const WaveData::Signal *signal) const {
  // Build a stack of all parents up to the root, or the matched signal scope,
  // whichever comes first.
  std::stack<const WaveData::SignalScope *> stack;
  const auto *scope = signal->scope;
  while (scope != nullptr) {
    stack.push(scope);
    if (scope == matched_signal_scope_) break;
    scope = scope->parent;
  }
  // Traverse the stack down the design tree hierarcy.
  const UHDM::any *design_scope = matched_design_scope_;
  while (design_scope != nullptr && !stack.empty()) {
    auto *signal_scope = stack.top();
    // Running the search as an anonoymous lambda allows for an easier exit out
    // of next loops when the hierarchy item is found. An alternative would be a
    // goto statement.
    [&] {
      if (ScopeMatch(design_scope->VpiName(), signal_scope->name)) {
        return;
      } else if (design_scope->VpiType() == vpiModule) {
        const auto *m = dynamic_cast<const UHDM::module_inst *>(design_scope);
        if (m->Modules() != nullptr) {
          for (const auto *sub : *m->Modules()) {
            if (ScopeMatch(sub->VpiName(), signal_scope->name)) {
              design_scope = sub;
              return;
            }
          }
        }
        if (m->Gen_scope_arrays() != nullptr) {
          for (const auto *ga : *m->Gen_scope_arrays()) {
            // Surelog always has an unrolled list of gen scope arrays with a
            // single generate scope as the only child.
            const auto *g = (*ga->Gen_scopes())[0];
            if (ScopeMatch(ga->VpiName(), signal_scope->name)) {
              design_scope = g;
              return;
            }
          }
        }
      } else if (design_scope->VpiType() == vpiGenScope) {
        // Nearly identical code to the above, but the Surelog classes don't
        // have this virtualized.
        const auto *g = dynamic_cast<const UHDM::gen_scope *>(design_scope);
        if (g->Modules() != nullptr) {
          for (const auto *sub : *g->Modules()) {
            if (ScopeMatch(sub->VpiName(), signal_scope->name)) {
              design_scope = sub;
              return;
            }
          }
        }
        if (g->Gen_scope_arrays() != nullptr) {
          for (const auto *ga : *g->Gen_scope_arrays()) {
            const auto *g = (*ga->Gen_scopes())[0];
            if (ScopeMatch(ga->VpiName(), signal_scope->name)) {
              design_scope = g;
              return;
            }
          }
        }
      }
      // Not considering anything else
      design_scope = nullptr;
    }();
    stack.pop();
  }
  // Bail out if there is no matching design.
  if (design_scope == nullptr) return nullptr;
  // Helper to match without [n] suffix, since the design does not contain
  // unrolled net arrays.
  auto array_match = [](std::string_view val,
                        std::string_view val_with_suffix) {
    const auto pos = val_with_suffix.find(val);
    if (pos == std::string::npos) return false;
    if (val.size() == val_with_suffix.size()) return true;
    return val_with_suffix[val.size()] == '[';
  };
  // Look for nets, variables.
  if (design_scope->VpiType() == vpiModule) {
    const auto *m = dynamic_cast<const UHDM::module_inst *>(design_scope);
    if (m->Nets() != nullptr) {
      for (const auto *n : *m->Nets()) {
        if (n->VpiName() == signal->name) return n;
      }
    }
    if (m->Variables() != nullptr) {
      for (const auto *v : *m->Variables()) {
        if (v->VpiName() == signal->name) return v;
      }
    }
    if (m->Array_nets() != nullptr) {
      for (const auto *a : *m->Array_nets()) {
        if (array_match(a->VpiName(), signal->name)) return a;
      }
    }
    if (m->Array_vars() != nullptr) {
      for (const auto *a : *m->Array_vars()) {
        if (array_match(a->VpiName(), signal->name)) return a;
      }
    }
  } else if (design_scope->VpiType() == vpiGenScope) {
    const auto *g = dynamic_cast<const UHDM::gen_scope *>(design_scope);
    if (g->Nets() != nullptr) {
      for (const auto *n : *g->Nets()) {
        if (n->VpiName() == signal->name) return n;
      }
    }
    if (g->Variables() != nullptr) {
      for (const auto *v : *g->Variables()) {
        if (v->VpiName() == signal->name) return v;
      }
    }
    if (g->Array_nets() != nullptr) {
      for (const auto *a : *g->Array_nets()) {
        if (array_match(a->VpiName(), signal->name)) return a;
      }
    }
    if (g->Array_vars() != nullptr) {
      for (const auto *a : *g->Array_vars()) {
        if (array_match(a->VpiName(), signal->name)) return a;
      }
    }
  }
  return nullptr;
}

} // namespace sv
