#pragma once

#include "external/fst/fstapi.h"
#include "wave_data.h"

namespace sv {

// FST implementation of WaveData, based on the FST library from GTKWave.
class FstWaveData : public WaveData {
 public:
  explicit FstWaveData(const std::string &file_name);
  ~FstWaveData() override;
  int Log10TimeUnits() const final;
  std::pair<uint64_t, uint64_t> TimeRange() const final;
  void LoadSignalSamples(const std::vector<const Signal *> &signals,
                         uint64_t start_time, uint64_t end_time) const final;
  void Reload() final;

 private:
  void ReadScopes();
  // The FST library is written in C and uses a lot of untyped handles.
  void *reader_ = nullptr;
};

} // namespace sv
