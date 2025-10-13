#pragma once

#include "absl/container/flat_hash_map.h"
#include "vcd_tokenizer.h"
#include "wave_data.h"
#include <stack>

namespace sv {

// VCD implementation of the WaveData interface. Full blown parser.
class VcdWaveData : public WaveData {
 public:
  static absl::StatusOr<std::unique_ptr<VcdWaveData>> Create(const std::string &file_name,
                                                             bool keep_glitches);
  static void PrintLoadProgress(bool b) { print_progress_ = b; }
  int Log10TimeUnits() const final { return time_units_; }
  std::pair<uint64_t, uint64_t> TimeRange() const final { return time_range_; }
  void LoadSignalSamples(const std::vector<const Signal *> &signals,
                         uint64_t start_time, uint64_t end_time) const final;
  absl::Status Reload() final;

 private:
  VcdWaveData(const std::string &file_name, bool keep_glitches);
  // Parse and discard tokens until and $end is encountered.
  absl::Status Parse();
  absl::Status ParseToEofCommand();
  absl::Status ParseVariable();
  absl::Status ParseScope();
  absl::Status ParseUpScope();
  absl::Status ParseTimescale();
  absl::Status ParseSimCommands();

  // Identifier codes vs IDs
  absl::flat_hash_map<std::string, uint32_t> signal_id_by_code_;
  std::pair<uint64_t, uint64_t> time_range_ = {0, 0};
  int time_units_;
  std::unique_ptr<VcdTokenizer> tokenizer_;

  // State while parsing header. Not used otherwise.
  std::stack<SignalScope *> scope_stack_;
  uint32_t current_id_ = 0;

  // Progress printf on the console.
  static bool print_progress_;
};

} // namespace sv
