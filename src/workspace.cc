#include "workspace.h"
#include "slang_utils.h"

#include <iostream>
#include <stack>
#include <stdexcept>

namespace sv {

bool Workspace::ParseDesign(int argc, char *argv[]) {
  // Additional options from simview:
  std::optional<bool> show_help;
  std::optional<std::string> waves_file;
  std::optional<bool> keep_glitches = false;
  slang_driver_.cmdLine.add("-h,--help", show_help, "Display available options");
  slang_driver_.cmdLine.add("-w,--waves", waves_file,
                            "Waves file to load. Supported formats are FST and VCD.");
  slang_driver_.cmdLine.add("--keep_glitches", keep_glitches,
                            "Retain 0-time transitions in the wave data. Normally pruned.");

  slang_driver_.addStandardArgs();
  if (!slang_driver_.parseCommandLine(argc, argv)) return false;
  if (show_help == true) {
    std::cout << slang_driver_.cmdLine.getHelpText(
                     "simview: a terminal-based verilog design browser and waves viewer.")
              << "\n";
    return 0;
  }
  if (slang_driver_.processOptions()) {
    std::cout << "Parsing files...\n";
    if (!slang_driver_.parseAllSources()) return false;
    slang_compilation_ = slang_driver_.createCompilation();
    design_root_ = &slang_compilation_->getRoot();
    const bool success = slang_driver_.reportDiagnostics(/* quiet */ false);
    // Give the user a chance to see any errors before proceeding.
    if (!success) {
      std::cout << "Errors encountered, press Enter to continue anyway...\n";
      std::cin.get();
    }
  }

  if (waves_file) {
    try {
      wave_data_ = WaveData::ReadWaveFile(*waves_file, *keep_glitches);
    } catch (std::runtime_error &e) {
      std::cout << "Problem reading waves: " << e.what() << "\n";
      return false;
    }
    // Try to match the two up.
    sv::Workspace::Get().TryMatchDesignWithWaves();
  }

  return true;
}

void Workspace::TryMatchDesignWithWaves() {
  // Don't do anything unless there are both waves and design.
  //  if (wave_data_ == nullptr || design_ == nullptr) return;
  //  // Look for a scope with stuff in it.
  //  std::vector<const WaveData::SignalScope *> signal_scopes;
  //  for (const auto &root_scope : wave_data_->Roots()) {
  //    signal_scopes.push_back(&root_scope);
  //    for (const auto &sub : root_scope.children) {
  //      signal_scopes.push_back(&sub);
  //      for (const auto &subsub : sub.children) {
  //        signal_scopes.push_back(&subsub);
  //      }
  //    }
  //  }
  //  // Okay, good enough. Now try to build a list of design scopes that look
  //  // interesting. Not considering generate blocks here for the auto-detect.
  //  std::vector<const UHDM::any *> design_scopes;
  //  for (const auto *top : *design_->TopModules()) {
  //    design_scopes.push_back(top);
  //    // If the top module has some guts, add it's modules too.
  //    if (top->Modules() != nullptr && !top->Modules()->empty()) {
  //      for (const auto *sub : *top->Modules()) {
  //        design_scopes.push_back(sub);
  //        if (sub->Modules() != nullptr && !sub->Modules()->empty()) {
  //          for (const auto *subsub : *sub->Modules()) {
  //            design_scopes.push_back(subsub);
  //          }
  //        }
  //      }
  //    }
  //  }
  //  // Try to match the two candidate scopes against each other by looking for
  //  // matching signals.
  //  int max_common = 0;
  //  for (const auto *design_scope : design_scopes) {
  //    const auto *m = dynamic_cast<const UHDM::module_inst *>(design_scope);
  //    for (const auto *signal_scope : signal_scopes) {
  //      if (signal_scope->signals.empty()) continue;
  //      int num_common_signals = 0;
  //      if (m->Nets() != nullptr) {
  //        for (const auto *n : *m->Nets()) {
  //          for (const auto &s : signal_scope->signals) {
  //            if (n->VpiName() == s.name) {
  //              num_common_signals++;
  //            }
  //          }
  //        }
  //      }
  //      if (m->Variables() != nullptr) {
  //        for (const auto *v : *m->Variables()) {
  //          for (const auto &s : signal_scope->signals) {
  //            if (v->VpiName() == s.name) {
  //              num_common_signals++;
  //            }
  //          }
  //        }
  //      }
  //      // Save the best result.
  //      if (num_common_signals > max_common) {
  //        max_common = num_common_signals;
  //        // TODO: re-enable this when above is all slang-ified.
  //        // matched_design_scope_ = design_scope;
  //        matched_signal_scope_ = signal_scope;
  //      }
  //    }
  //  }
}

std::vector<const WaveData::Signal *>
Workspace::DesignToSignals(const slang::ast::Symbol *item) const {
  if (matched_signal_scope_ == nullptr) return {};
  std::vector<const WaveData::Signal *> signals;
  // Make sure it's actually something that would have ended up in a wave.
  if (!IsTraceable(item)) return signals;
  // Build a stack of all parents up to the top module, or the matched design
  // scope, whichever comes first.
  // std::stack<const UHDM::any *> stack;
  // const UHDM::any *parent = item->VpiParent();
  // while (parent != nullptr) {
  //  // Drop out any genscopes, there's always a genscope_array above it.
  //  if (parent->VpiType() == vpiGenScope) parent = parent->VpiParent();
  //  // TODO: Re-instante when slang-ified.
  //  // if (parent == matched_design_scope_) break;
  //  stack.push(parent);
  //  parent = parent->VpiParent();
  //}

  // const WaveData::SignalScope *signal_scope = matched_signal_scope_;
  // while (!stack.empty()) {
  //   auto *design_scope = stack.top();
  //   bool found = false;
  //   if (ScopeMatch(design_scope->VpiName(), signal_scope->name)) {
  //     found = true;
  //   } else {
  //     for (const auto &sub : signal_scope->children) {
  //       if (ScopeMatch(design_scope->VpiName(), sub.name)) {
  //         signal_scope = &sub;
  //         found = true;
  //         break;
  //       }
  //     }
  //   }
  //   // Total abort if there is no matching scope in the wave data.
  //   if (!found) return signals;
  //   stack.pop();
  // }
  //// Now add any signals in the drilled-down scope that match the net name
  //// exactly, with optional square bracket suffixes.
  // const std::string net_name = StripWorklib(item->VpiName());
  // for (const auto &signal : signal_scope->signals) {
  //   auto pos = signal.name.find(StripWorklib(item->VpiName()));
  //   if (pos != 0) continue;
  //   if (signal.name.size() > net_name.size() && signal.name[net_name.size()] != '[') {
  //     continue;
  //   }
  //   signals.push_back(&signal);
  // }

  return signals;
}

const slang::ast::Symbol *Workspace::SignalToDesign(const WaveData::Signal *signal) const {
  if (matched_design_scope_ == nullptr) return nullptr;
  // Build a stack of all parents up to the root, or the matched signal scope,
  // whichever comes first.
  std::stack<const WaveData::SignalScope *> stack;
  const auto *scope = signal->scope;
  while (scope != nullptr) {
    if (scope == matched_signal_scope_) break;
    stack.push(scope);
    scope = scope->parent;
  }
  // Traverse the stack down the design tree hierarcy.
  // TODO: slang-ify this.
  // const UHDM::any *design_scope = matched_design_scope_;
  // const UHDM::any *design_scope = nullptr;
  // while (design_scope != nullptr && !stack.empty()) {
  //  auto *signal_scope = stack.top();
  //  // Running the search as an anonoymous lambda allows for an easier exit out
  //  // of next loops when the hierarchy item is found. An alternative would be a
  //  // goto statement.
  //  [&] {
  //    if (ScopeMatch(design_scope->VpiName(), signal_scope->name)) {
  //      return;
  //    } else if (design_scope->VpiType() == vpiModule) {
  //      const auto *m = dynamic_cast<const UHDM::module_inst *>(design_scope);
  //      if (m->Modules() != nullptr) {
  //        for (const auto *sub : *m->Modules()) {
  //          if (ScopeMatch(sub->VpiName(), signal_scope->name)) {
  //            design_scope = sub;
  //            return;
  //          }
  //        }
  //      }
  //      if (m->Gen_scope_arrays() != nullptr) {
  //        for (const auto *ga : *m->Gen_scope_arrays()) {
  //          // Surelog always has an unrolled list of gen scope arrays with a
  //          // single generate scope as the only child.
  //          const auto *g = (*ga->Gen_scopes())[0];
  //          if (ScopeMatch(ga->VpiName(), signal_scope->name)) {
  //            design_scope = g;
  //            return;
  //          }
  //        }
  //      }
  //    } else if (design_scope->VpiType() == vpiGenScope) {
  //      // Nearly identical code to the above, but the Surelog classes don't
  //      // have this virtualized.
  //      const auto *g = dynamic_cast<const UHDM::gen_scope *>(design_scope);
  //      if (g->Modules() != nullptr) {
  //        for (const auto *sub : *g->Modules()) {
  //          if (ScopeMatch(sub->VpiName(), signal_scope->name)) {
  //            design_scope = sub;
  //            return;
  //          }
  //        }
  //      }
  //      if (g->Gen_scope_arrays() != nullptr) {
  //        for (const auto *ga : *g->Gen_scope_arrays()) {
  //          const auto *g = (*ga->Gen_scopes())[0];
  //          if (ScopeMatch(ga->VpiName(), signal_scope->name)) {
  //            design_scope = g;
  //            return;
  //          }
  //        }
  //      }
  //    }
  //    // Not considering anything else
  //    design_scope = nullptr;
  //  }();
  //  stack.pop();
  //}
  // Bail out if there is no matching design.
  // if (design_scope == nullptr) return nullptr;
  //// Helper to match without [n] suffix, since the design does not contain
  //// unrolled net arrays.
  // auto array_match = [](std::string_view val, std::string_view val_with_suffix) {
  //   const auto pos = val_with_suffix.find(val);
  //   if (pos == std::string::npos) return false;
  //   if (val.size() == val_with_suffix.size()) return true;
  //   return val_with_suffix[val.size()] == '[';
  // };
  //// Look for nets, variables.
  // if (design_scope->VpiType() == vpiModule) {
  //   const auto *m = dynamic_cast<const UHDM::module_inst *>(design_scope);
  //   if (m->Nets() != nullptr) {
  //     for (const auto *n : *m->Nets()) {
  //       if (n->VpiName() == signal->name) return n;
  //     }
  //   }
  //   if (m->Variables() != nullptr) {
  //     for (const auto *v : *m->Variables()) {
  //       if (v->VpiName() == signal->name) return v;
  //     }
  //   }
  //   if (m->Array_nets() != nullptr) {
  //     for (const auto *a : *m->Array_nets()) {
  //       if (array_match(a->VpiName(), signal->name)) return a;
  //     }
  //   }
  //   if (m->Array_vars() != nullptr) {
  //     for (const auto *a : *m->Array_vars()) {
  //       if (array_match(a->VpiName(), signal->name)) return a;
  //     }
  //   }
  // } else if (design_scope->VpiType() == vpiGenScope) {
  //   const auto *g = dynamic_cast<const UHDM::gen_scope *>(design_scope);
  //   if (g->Nets() != nullptr) {
  //     for (const auto *n : *g->Nets()) {
  //       if (n->VpiName() == signal->name) return n;
  //     }
  //   }
  //   if (g->Variables() != nullptr) {
  //     for (const auto *v : *g->Variables()) {
  //       if (v->VpiName() == signal->name) return v;
  //     }
  //   }
  //   if (g->Array_nets() != nullptr) {
  //     for (const auto *a : *g->Array_nets()) {
  //       if (array_match(a->VpiName(), signal->name)) return a;
  //     }
  //   }
  //   if (g->Array_vars() != nullptr) {
  //     for (const auto *a : *g->Array_vars()) {
  //       if (array_match(a->VpiName(), signal->name)) return a;
  //     }
  //   }
  // }
  return nullptr;
}

void Workspace::SetMatchedDesignScope(const slang::ast::Symbol *s) {
  matched_design_scope_ = s;
  // ApplyDesignData(matched_design_scope_, matched_signal_scope_);
}

void Workspace::SetMatchedSignalScope(const WaveData::SignalScope *s) {
  matched_signal_scope_ = s;
  // ApplyDesignData(matched_design_scope_, matched_signal_scope_);
}

} // namespace sv
