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

 private:
  void *reader_ = nullptr;
};

} // namespace sv
