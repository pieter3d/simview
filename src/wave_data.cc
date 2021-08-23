#include "wave_data.h"
#include "fst_wave_data.h"
#include <filesystem>

namespace sv {

std::unique_ptr<WaveData> WaveData::ReadWaveFile(const std::string &file_name) {
  auto ext = std::filesystem::path(file_name).extension();
  if (ext.string() == ".fst" || ext == ".FST") {
    return std::make_unique<FstWaveData>(file_name);
  }
  return nullptr;
}

} // namespace sv
