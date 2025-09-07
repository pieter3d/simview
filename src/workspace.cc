#include "workspace.h"
#include "slang/analysis/AnalysisManager.h"
#include "slang/ast/ASTVisitor.h"
#include "slang/ast/Compilation.h"
#include "slang/ast/symbols/BlockSymbols.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/ast/symbols/InstanceSymbols.h"
#include "slang/ast/symbols/MemberSymbols.h"
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
    std::cout << "Elaborating...\n";
    slang_compilation_ = slang_driver_->createCompilation();
    // TODO: This freezes the design and somehow makes traversal throw exceptions.
    // std::cout << "Analyzing...\n";
    // slang_analysis_ = slang_driver_->runAnalysis(*slang_compilation_);
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
    stack.push(scope);
    if (scope == matched_signal_scope_) break;
    scope = scope->parent;
  }
  const slang::ast::Symbol *design_top = &matched_design_scope_->asSymbol();
  // Use the instance as the traversal start.
  if (const auto *body = design_top->as_if<slang::ast::InstanceBodySymbol>()) {
    design_top = body->parentInstance;
  }
  // Traverse the stack down the design tree hierarcy.
  const slang::ast::Scope *design_scope = nullptr;
  design_top->visit(slang::ast::makeVisitor(
      [&](auto &visitor, const slang::ast::InstanceSymbol &inst) {
        if (stack.empty()) return;
        // Handle array instances this way.
        if (inst.getArrayName() == stack.top()->name) stack.pop();
        // Traverse deeper into the tree if there are still signal levels left to match.
        if (!stack.empty()) {
          visitor.visitDefault(inst);
        } else {
          // Stack was just emptied, this must be the matching scope of the signal.
          design_scope = &inst.body;
        }
      },
      [&](auto &visitor, const slang::ast::GenerateBlockSymbol &gen) {
        if (stack.empty()) return;
        if (gen.name == stack.top()->name) stack.pop();
        if (!stack.empty()) {
          visitor.visitDefault(gen);
        } else {
          design_scope = &gen;
        }
      }));
  // The signal scope stack should be empty if the design hierarchy was matched.
  if (!stack.empty()) return nullptr;
  // If not found, also abort.
  if (design_scope == nullptr) return nullptr;
  // Helper to match without [n] suffix, since the design does not contain
  // unrolled net arrays.
  auto array_match = [](std::string_view val, std::string_view val_with_suffix) {
    const auto pos = val_with_suffix.find(val);
    if (pos == std::string::npos) return false;
    if (val.size() == val_with_suffix.size()) return true;
    return val_with_suffix[val.size()] == '[' || val_with_suffix.substr(val.size()) == " [";
  };
  // Find nets and variables withing the discovered scope.
  const slang::ast::Symbol *sym = nullptr;
  design_scope->asSymbol().visit(slang::ast::makeVisitor(
      // Do nothing for instances and generate blocks, to avoid going deeper.
      [&](auto &visitor, const slang::ast::InstanceSymbol &) {},
      [&](auto &visitor, const slang::ast::GenvarSymbol &) {},
      [&](auto &visitor, const slang::ast::NetSymbol &net) {
        if (sym == nullptr && array_match(net.name, signal->name)) sym = &net;
      },
      [&](auto &visitor, const slang::ast::VariableSymbol &var) {
        if (sym == nullptr && array_match(var.name, signal->name)) sym = &var;
      }));
  return sym;
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
