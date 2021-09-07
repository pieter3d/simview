#include "wave_data.h"
#include "absl/strings/str_split.h"
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

std::optional<const WaveData::Signal *>
WaveData::PathToSignal(const std::string &path) const {
  std::vector<std::string> levels = absl::StrSplit(path, ".");
  const std::vector<SignalScope> *candidates = &roots_;
  const SignalScope *candidate = nullptr;
  int level_idx = 0;
  for (auto &level : levels) {
    // Match against the signals for the last element.
    if (level_idx == levels.size() - 1) {
      for (const auto &signal : candidate->signals) {
        if (signal.name == level) return &signal;
      }
    } else {
      level_idx++;
      for (const auto &scope : *candidates) {
        if (scope.name == level) {
          candidates = &scope.children;
          candidate = &scope;
          continue;
        }
      }
      // No matching level, so abort.
      if (candidate == nullptr) break;
    }
  }
  return std::nullopt;
}

std::string WaveData::SignalToPath(const WaveData::Signal *signal) {
  std::string path = signal->name;
  auto scope = signal->scope;
  while (scope != nullptr) {
    path = scope->name + "." + path;
    scope = scope->parent;
  }
  return path;
}

} // namespace sv
