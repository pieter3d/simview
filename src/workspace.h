#pragma once

#include "slang/ast/Compilation.h"
#include "slang/ast/symbols/CompilationUnitSymbols.h"
#include "slang/driver/Driver.h"
#include "wave_data.h"
#include <cstdint>
#include <vector>

namespace sv {

// Holds the application-level global data, namely the slang design and / or the wave file.
class Workspace {
 public:
  // Obtain the singleton.
  static Workspace &Get() {
    static Workspace w;
    return w;
  }

  // Parse the design using command line arguments for Surelog. Return true on success.
  bool ParseDesign(int argc, char *argv[]);

  const slang::ast::RootSymbol *Design() const { return design_root_; }

  // Find the definition of the module that contains the given item.
  uint64_t &WaveCursorTime() { return wave_cursor_time_; }

  const slang::SourceManager *SourceManager() const {
    return slang_compilation_->getSourceManager();
  }

  const WaveData *Waves() const { return wave_data_.get(); }

  // Non-const version allows for reload.
  WaveData *Waves() { return wave_data_.get(); }

  void TryMatchDesignWithWaves();

  const slang::ast::Symbol *MatchedDesignScope() const { return matched_design_scope_; }

  const WaveData::SignalScope *MatchedSignalScope() const { return matched_signal_scope_; }

  void SetMatchedDesignScope(const slang::ast::Symbol *s);

  void SetMatchedSignalScope(const WaveData::SignalScope *s);

  std::vector<const WaveData::Signal *> DesignToSignals(const slang::ast::Symbol *item) const;

  const slang::ast::Symbol *SignalToDesign(const WaveData::Signal *signal) const;

 private:
  // Singleton
  Workspace() = default;

  slang::driver::Driver slang_driver_;
  std::unique_ptr<slang::ast::Compilation> slang_compilation_;
  const slang::ast::RootSymbol *design_root_;
  const slang::ast::Symbol *matched_design_scope_ = nullptr;
  std::unique_ptr<WaveData> wave_data_;
  const WaveData::SignalScope *matched_signal_scope_ = nullptr;
  // Wave time is used in source too, so it's held here.
  uint64_t wave_cursor_time_ = 0;
};

} // namespace sv
