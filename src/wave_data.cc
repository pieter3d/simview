#include "wave_data.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "fst_wave_data.h"
#include "vcd_wave_data.h"
#include <filesystem>
#include <optional>

namespace sv {

absl::StatusOr<std::unique_ptr<WaveData>> WaveData::ReadWaveFile(const std::string &file_name,
                                                                 bool keep_glitches) {
  std::string ext = std::filesystem::path(file_name).extension().string();
  std::transform(ext.begin(), ext.end(), ext.begin(), [](char ch) { return std::tolower(ch); });
  if (ext == ".fst") {
    return FstWaveData::Create(file_name, keep_glitches);
  } else if (ext == ".vcd") {
    return VcdWaveData::Create(file_name, keep_glitches);
  }
  return absl::UnimplementedError("Unsupported wave type.");
}

std::optional<WaveData::Signal *> WaveData::PathToSignal(std::string_view path) {
  std::vector<std::string> levels = absl::StrSplit(path, '.');
  std::vector<SignalScope> *candidates = &roots_;
  SignalScope *candidate = nullptr;
  int level_idx = 0;
  for (auto &level : levels) {
    // Match against the signals for the last element.
    if (level_idx == levels.size() - 1) {
      if (candidate == nullptr) break;
      for (Signal &signal : candidate->signals) {
        if (signal.name == level) return &signal;
      }
    } else {
      level_idx++;
      for (SignalScope &scope : *candidates) {
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

std::optional<const WaveData::Signal *> WaveData::PathToSignal(std::string_view path) const {
  return const_cast<WaveData *>(this)->PathToSignal(path);
}

std::string WaveData::SignalToPath(const WaveData::Signal *signal) {
  std::string path = signal->name;
  auto scope = signal->scope;
  while (scope != nullptr) {
    path = absl::StrCat(scope->name, ".", path);
    scope = scope->parent;
  }
  return path;
}

std::optional<std::string_view> WaveData::GetEnumLabel(int enum_id, std::string_view val) const {
  if (enum_id < 0) return std::nullopt;
  auto enum_it = enums_.find(enum_id);
  if (enum_it == enums_.end()) return std::nullopt;
  if (auto it = enum_it->second.find(val); it != enum_it->second.end()) {
    return it->second;
  }
  return std::nullopt;
}

void WaveData::LoadSignalSamples(const Signal *signal, uint64_t start_time,
                                 uint64_t end_time) const {
  // Use the batch version.
  std::vector<const Signal *> sigs({signal});
  LoadSignalSamples(sigs, start_time, end_time);
}

void WaveData::BuildParents() {
  std::function<void(SignalScope *)> recurse_assign_parents = [&](SignalScope *scope) {
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

int WaveData::FindSampleIndex(uint64_t time, const Signal *signal, int left, int right) const {
  auto &wave = waves_[signal->id];
  // Binary search for the right sample.
  if (wave.empty() || right < left) return -1;
  if (right - left <= 1) {
    // Use the new value when on the same time.
    return time < wave[right].time ? left : right;
  }
  const int mid = (left + right) / 2;
  if (time > wave[mid].time) {
    return FindSampleIndex(time, signal, mid, right);
  } else {
    return FindSampleIndex(time, signal, left, mid);
  }
}

int WaveData::FindSampleIndex(uint64_t time, const Signal *signal) const {
  return FindSampleIndex(time, signal, 0, waves_[signal->id].size() - 1);
}

} // namespace sv
