#pragma once

#include "wave_data.h"
#include <surelog/surelog.h>
#include <uhdm/headers/design.h>
#include <unordered_map>
#include <vector>

namespace sv {

// Holds the application-level global data, namely the UHDM design and / or the
// wave file.
class Workspace {
 public:
  // Obtain the singleton.
  static Workspace &Get() {
    static Workspace w;
    return w;
  }

  // Parse the design using command line arguments for Surelog.
  // Return true on success.
  bool ParseDesign(int argc, const char *argv[]);
  // Attempt to parse wave file. Return true on success.
  bool ReadWaves(const std::string &wave_file);
  const UHDM::design *Design() const { return design_; }
  // Find the definition of the module that contains the given item.
  const UHDM::module *GetDefinition(const UHDM::module *m);
  uint64_t &WaveCursorTime() { return wave_cursor_time_; }

  void AddIncludeDir(std::string d) { include_paths_.push_back(d); }
  const auto &IncludeDirs() const { return include_paths_; }
  const WaveData *Waves() const { return wave_data_.get(); }

  void TryMatchDesignWithWaves();
  auto MatchedDesignScope() const { return matched_design_scope_; }
  auto MatchedSignalScope() const { return matched_signal_scope_; }
  void SetMatchedDesignScope(const UHDM::any *s) { matched_design_scope_ = s; }
  void SetMatchedSignalScope(const WaveData::SignalScope *s) {
    matched_signal_scope_ = s;
  }
  std::vector<const WaveData::Signal *>
  DesignToSignals(const UHDM::any *item) const;
  const UHDM::any *SignalToDesign(const WaveData::Signal *signal) const;

 private:
  // Singleton
  Workspace() {}
  ~Workspace();
  // Track all definitions of any given module instance.
  // This serves as a cache to avoid iterating over the
  // design's list of all module definitions.
  std::unordered_map<std::string, const UHDM::module *> module_defs_;
  UHDM::design *design_ = nullptr;
  SURELOG::SymbolTable symbol_table_;
  SURELOG::scompiler *compiler_ = nullptr;
  std::vector<std::string> include_paths_;
  std::unique_ptr<WaveData> wave_data_;
  const WaveData::SignalScope *matched_signal_scope_ = nullptr;
  const UHDM::any *matched_design_scope_ = nullptr;
  // Wave time is used in source too, so it's held here.
  uint64_t wave_cursor_time_ = 0;
};

} // namespace sv
