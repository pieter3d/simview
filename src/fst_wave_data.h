#pragma once

#include "external/fst/fstapi.h"
#include "wave_data.h"

namespace sv {

class FstWaveData : public WaveData {
 public:
  FstWaveData(const std::string &file_name);
  ~FstWaveData();

 private:
  void *reader_ = nullptr;
};

} // namespace sv
