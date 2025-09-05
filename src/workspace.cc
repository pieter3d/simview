#include "workspace.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Compilation.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/VariableSymbols.h"
#include "slang/driver/Driver.h"
#include "slang_utils.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <stack>
#include <stdexcept>

namespace sv {

Workspace::Workspace() = default;
Workspace::~Workspace() = default;

const slang::SourceManager *Workspace::SourceManager() const {
  return slang_compilation_->getSourceManager();
}

bool Workspace::ParseDesign(int argc, char *argv[]) {
  // Avoid invoking the slang compiler if only wave-reading options are specified.
  const bool try_read_design =
      !(argc == 3 && (std::strcmp(argv[1], "-w") == 0 || std::strcmp(argv[1], "--waves") == 0));
  slang_driver_ = std::make_unique<slang::driver::Driver>();
  // Additional options from simview:
  std::optional<bool> show_help;
  std::optional<std::string> waves_file;
  std::optional<bool> keep_glitches = false;
  slang_driver_->cmdLine.add("-h,--help", show_help, "Display available options");
  slang_driver_->cmdLine.add("-w,--waves", waves_file,
                             "Waves file to load. Supported formats are FST and VCD.");
  slang_driver_->cmdLine.add("--keep_glitches", keep_glitches,
                             "Retain 0-time transitions in the wave data. Normally pruned.");

  slang_driver_->addStandardArgs();
  if (!slang_driver_->parseCommandLine(argc, argv)) return false;
  if (show_help == true) {
    std::cout << slang_driver_->cmdLine.getHelpText(
                     "simview: a terminal-based verilog design browser and waves viewer.")
              << "\n";
    return 0;
  }
  if (try_read_design && slang_driver_->processOptions()) {
    std::cout << "Parsing files...\n";
    if (!slang_driver_->parseAllSources()) return false;
    slang_compilation_ = slang_driver_->createCompilation();
    // This print all tops, and collects diagnostics.
    slang_driver_->reportCompilation(*slang_compilation_, /* quiet */ false);
    const bool success = slang_driver_->reportDiagnostics(/* quiet */ false);
    // Give the user a chance to see any errors before proceeding.
    if (!success) {
      std::cout << "Errors encountered, press Enter to continue anyway...\n";
      std::cin.get();
    }
    design_root_ = &slang_compilation_->getRoot();
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
  if (wave_data_ == nullptr || design_root_ == nullptr) return;
  // Look for a scope with stuff in it, go 3 levels in.
  std::vector<const WaveData::SignalScope *> signal_scopes;
  for (const WaveData::SignalScope &root_scope : wave_data_->Roots()) {
    signal_scopes.push_back(&root_scope);
    for (const WaveData::SignalScope &sub : root_scope.children) {
      signal_scopes.push_back(&sub);
      for (const WaveData::SignalScope &subsub : sub.children) {
        signal_scopes.push_back(&subsub);
      }
    }
  }
  // Okay, good enough. Now try to build a list of design scopes that aren't too far from the top.
  std::vector<const slang::ast::Scope *> design_scopes;
  int depth = 0;
  for (const slang::ast::InstanceSymbol *top : design_root_->topInstances) {
    design_scopes.push_back(&top->body);
    top->visit(slang::ast::makeVisitor(
        [&](auto &visitor, const slang::ast::InstanceSymbol &inst) {
          design_scopes.push_back(&inst.body);
          if (depth < 3) {
            depth++;
            visitor.visitDefault(inst);
            depth--;
          }
        },
        [&](auto &visitor, const slang::ast::GenerateBlockSymbol &gen) {
          design_scopes.push_back(&gen);
          if (depth < 3) {
            depth++;
            visitor.visitDefault(gen);
            depth--;
          }
        }));
  }

  // Try to match the two candidate scopes against each other by looking for
  // matching signals.
  int max_common = 0;
  // Default scope is just the first root.
  if (!wave_data_->Roots().empty()) matched_signal_scope_ = &wave_data_->Roots()[0];
  for (const slang::ast::Scope *design_scope : design_scopes) {
    for (const WaveData::SignalScope *signal_scope : signal_scopes) {
      if (signal_scope->signals.empty()) continue;
      int num_common_signals = 0;
      for (const auto &net : design_scope->membersOfType<slang::ast::NetSymbol>()) {
        for (const auto &s : signal_scope->signals) {
          if (net.name == s.name) {
            num_common_signals++;
          }
        }
      }
      for (const auto &var : design_scope->membersOfType<slang::ast::VariableSymbol>()) {
        for (const auto &s : signal_scope->signals) {
          if (var.name == s.name) {
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

std::vector<const WaveData::Signal *>
Workspace::DesignToSignals(const slang::ast::Symbol *item) const {
  if (matched_signal_scope_ == nullptr) return {};
  std::vector<const WaveData::Signal *> signals;
  // Make sure it's actually something that would have ended up in a wave.
  if (!IsTraceable(item)) return signals;
  // Build a stack of all parents up to the top module, or the matched design scope, whichever comes
  // first.
  std::stack<const slang::ast::Scope *> stack;
  const slang::ast::Scope *parent = item->getParentScope();
  while (parent != nullptr) {
    if (parent == matched_design_scope_) break;
    stack.push(parent);
    parent = parent->asSymbol().getHierarchicalParent();
  }

  const WaveData::SignalScope *signal_scope = matched_signal_scope_;
  while (!stack.empty()) {
    const slang::ast::Scope *design_scope = stack.top();
    std::string_view scope_name;
    if (const auto *body = design_scope->asSymbol().as_if<slang::ast::InstanceBodySymbol>()) {
      scope_name = body->parentInstance->name;
    } else {
      scope_name = design_scope->asSymbol().name;
    }
    bool found = false;
    if (scope_name == signal_scope->name) {
      found = true;
    } else {
      for (const WaveData::SignalScope &sub : signal_scope->children) {
        if (scope_name == sub.name) {
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
  // Now add any signals in the drilled-down scope that match the net name exactly, with optional
  // square bracket suffixes.
  for (const auto &signal : signal_scope->signals) {
    auto pos = signal.name.find(item->name);
    if (pos != 0) continue;
    // If the next character starts an array index, accept it. Otherwise move on.
    if (signal.name.size() > item->name.size() &&
        signal.name.find_first_of(" [", item->name.size()) != item->name.size()) {
      continue;
    }
    signals.push_back(&signal);
  }

  return signals;
}

const slang::ast::Symbol *Workspace::SignalToDesign(const WaveData::Signal *signal) const {
  if (matched_design_scope_ == nullptr) return nullptr;
  // Build a stack of all parents up to the root, or the matched signal scope, whichever comes
  // first.
  std::stack<const WaveData::SignalScope *> stack;
  const auto *scope = signal->scope;
  while (scope != nullptr) {
    if (scope == matched_signal_scope_) break;
    stack.push(scope);
    scope = scope->parent;
  }
  // Traverse the stack down the design tree hierarcy.
  const slang::ast::Scope *design_scope = matched_design_scope_;
  matched_design_scope_->asSymbol().visit(slang::ast::makeVisitor(
        [&](auto &visitor, const slang::ast::InstanceSymbol &inst) {
        }));
  (void)design_scope;
  // TODO:finish
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

void Workspace::SetMatchedDesignScope(const slang::ast::Scope *s) {
  matched_design_scope_ = s;
  // ApplyDesignData(matched_design_scope_, matched_signal_scope_);
}

void Workspace::SetMatchedSignalScope(const WaveData::SignalScope *s) {
  matched_signal_scope_ = s;
  // ApplyDesignData(matched_design_scope_, matched_signal_scope_);
}

} // namespace sv
