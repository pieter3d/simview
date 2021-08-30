#pragma once

#include "external/fst/fstapi.h"
#include "wave_data.h"

namespace sv {

class FstWaveData : public WaveData {
 public:
  FstWaveData(const std::string &file_name);
  ~FstWaveData();
  int Log10TimeUnits() const final;
  std::pair<uint64_t, uint64_t> TimeRange() const final;
  std::string SignalValue(const Signal *signal, uint64_t time) const final;
  std::vector<Sample> SignalSamples(const Signal *signal, uint64_t start_time,
                                    uint64_t end_time) const final;
  std::vector<std::vector<Sample>>
  SignalSamples(const std::vector<const Signal *> &signals, uint64_t start_time,
                uint64_t end_time) const final;

 private:
  void *reader_ = nullptr;
};

} // namespace sv
