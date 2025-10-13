#pragma once

#include "external/libfst//src//fstapi.h"
#include "wave_data.h"

namespace sv {

// FST implementation of WaveData, based on the FST library from GTKWave.
class FstWaveData : public WaveData {
 public:
  static absl::StatusOr<std::unique_ptr<FstWaveData>> Create(const std::string &file_name,
                                                             bool keep_glitches);
  ~FstWaveData() override;
  int Log10TimeUnits() const final;
  std::pair<uint64_t, uint64_t> TimeRange() const final;
  void LoadSignalSamples(const std::vector<const Signal *> &signals, uint64_t start_time,
                         uint64_t end_time) const final;
  absl::Status Reload() final;

 private:
  FstWaveData(const std::string &file_name, bool keep_glitches);
  void ReadScopes();
  // The FST library is written in C and uses a lot of untyped handles.
  fstReaderContext *reader_ = nullptr;
};

} // namespace sv
