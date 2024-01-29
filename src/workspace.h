#pragma once

#include "absl/container/flat_hash_map.h"
#include "wave_data.h"
#include <Surelog/surelog.h>
#include <cstdint>
#include <uhdm/design.h>
#include <uhdm/module_inst.h>
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
  bool ReadWaves(const std::string &wave_file, bool keep_glitches);
  const UHDM::design *Design() const { return design_; }
  // Find the definition of the module that contains the given item.
  const UHDM::module_inst *GetDefinition(const UHDM::module_inst *m);
  uint64_t &WaveCursorTime() { return wave_cursor_time_; }

  void AddIncludeDir(std::string_view d) { include_paths_.push_back(d); }
  const auto &IncludeDirs() const { return include_paths_; }
  const WaveData *Waves() const { return wave_data_.get(); }
  // Non-const version allows for reload.
  WaveData *Waves() { return wave_data_.get(); }

  void TryMatchDesignWithWaves();
  auto MatchedDesignScope() const { return matched_design_scope_; }
  auto MatchedSignalScope() const { return matched_signal_scope_; }
  void SetMatchedDesignScope(const UHDM::any *s);
  void SetMatchedSignalScope(const WaveData::SignalScope *s);
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
  absl::flat_hash_map<std::string, const UHDM::module_inst *> module_defs_;
  UHDM::design *design_ = nullptr;
  SURELOG::SymbolTable symbol_table_;
  SURELOG::scompiler *compiler_ = nullptr;
  std::vector<std::string_view> include_paths_;
  std::unique_ptr<WaveData> wave_data_;
  const WaveData::SignalScope *matched_signal_scope_ = nullptr;
  const UHDM::any *matched_design_scope_ = nullptr;
  // Wave time is used in source too, so it's held here.
  uint64_t wave_cursor_time_ = 0;
};

} // namespace sv
