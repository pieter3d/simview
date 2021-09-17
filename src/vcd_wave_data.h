#pragma once

#include "vcd_tokenizer.h"
#include "wave_data.h"
#include <map>
#include <stack>
#include <unordered_map>

namespace sv {

// VCD implementation of the WaveData interface. Full blown parser.
class VcdWaveData : public WaveData {
 public:
  static void PrintLoadProgress(bool b) { print_progress_ = b; }
  explicit VcdWaveData(const std::string &file_name);
  int Log10TimeUnits() const final { return time_units_; }
  std::pair<uint64_t, uint64_t> TimeRange() const final { return time_range_; }
  void LoadSignalSamples(const std::vector<const Signal *> &signals,
                         uint64_t start_time, uint64_t end_time) const final;
  void Reload() final;

 private:
  // Parse and discard tokens until and $end is encountered.
  void Parse();
  void ParseToEofCommand();
  void ParseVariable();
  void ParseScope();
  void ParseUpScope();
  void ParseTimescale();
  void ParseSimCommands();

  // Identifier codes vs IDs
  std::unordered_map<std::string, uint32_t> signal_id_by_code_;
  std::pair<uint64_t, uint64_t> time_range_ = {0, 0};
  int time_units_;
  VcdTokenizer tokenizer_;

  // State while parsing header. Not used otherwise.
  std::stack<SignalScope *> scope_stack_;
  uint32_t current_id_ = 0;

  // Progress printf on the console.
  static bool print_progress_;
};

} // namespace sv
