#include "wave_data.h"
#include "absl/strings/str_split.h"
#include "fst_wave_data.h"
#include "vcd_wave_data.h"
#include <filesystem>

namespace sv {

std::unique_ptr<WaveData> WaveData::ReadWaveFile(const std::string &file_name) {
  std::string ext = std::filesystem::path(file_name).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](char ch) { return std::tolower(ch); });
  if (ext == ".fst") {
    return std::make_unique<FstWaveData>(file_name);
  } else if (ext == ".vcd") {
    return std::make_unique<VcdWaveData>(file_name);
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

void WaveData::LoadSignalSamples(const Signal *signal, uint64_t start_time,
                                 uint64_t end_time) const {
  // Use the batch version.
  std::vector<const Signal *> sigs({signal});
  LoadSignalSamples(sigs, start_time, end_time);
}

void WaveData::BuildParents() {
  std::function<void(SignalScope *)> recurse_assign_parents =
      [&](SignalScope *scope) {
        // Assign to all signals.
        for (auto &sig : scope->signals) {
          sig.scope = scope;
        }
        for (auto &sub : scope->children) {
          sub.parent = scope;
          recurse_assign_parents(&sub);
        }
      };
  for (auto &root : roots_) {
    recurse_assign_parents(&root);
  }
}

} // namespace sv
