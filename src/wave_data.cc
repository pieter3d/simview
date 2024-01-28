#include "wave_data.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_split.h"
#include "fst_wave_data.h"
#include "vcd_wave_data.h"
#include <filesystem>
#include <uhdm/module_inst.h>
#include <uhdm/net.h>
#include <uhdm/ref_typespec.h>
#include <uhdm/logic_typespec.h>
#include <uhdm/struct_net.h>
#include <uhdm/struct_typespec.h>
#include <uhdm/typespec.h>
#include <uhdm/typespec_member.h>

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
  std::vector<std::string> levels = absl::StrSplit(path, '.');
  const std::vector<SignalScope> *candidates = &roots_;
  const SignalScope *candidate = nullptr;
  int level_idx = 0;
  for (auto &level : levels) {
    // Match against the signals for the last element.
    if (level_idx == levels.size() - 1) {
      if (candidate == nullptr) break;
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
    path = absl::StrCat(scope->name, ".", path);
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

int WaveData::FindSampleIndex(uint64_t time, const Signal *signal, int left,
                              int right) const {
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

namespace {

// TODO: Incomplete.
void ApplyStruct(const UHDM::struct_typespec *struct_spec,
                 const WaveData::Signal &s, int msb) {
  for (const auto *member : *struct_spec->Members()) {
    if (member->Typespec()->Actual_typespec()->VpiType() == vpiLogicTypespec) {
      const auto *lts = dynamic_cast<const UHDM::logic_typespec *>(
          member->Typespec()->Actual_typespec());
      (void)lts;
      (void)s;
      (void)msb;
      // TODO: Figure out a way to determine member sizes.

      // TODO: For now just rename the signal to see what can be discovered.
      // const_cast<WaveData::Signal &>(s).name += absl::StrFormat(
      //     "[name=%s l=%s, r=%s]", std::string(member->VpiName()).c_str(),
      //     lts->VpiName(), "");
    }
  }
}

} // namespace

void ApplyDesignData(const UHDM::any *design_scope,
                     const WaveData::SignalScope *signal_scope) {
  return; // TODO: Finish implementing this.
  if (design_scope == nullptr) return;
  if (signal_scope == nullptr) return;
  // Make sure the design scope is a module.
  if (design_scope->VpiType() != vpiModule) return;

  std::function<void(const UHDM::module_inst *, const WaveData::SignalScope *)>
      apply = [&](const UHDM::module_inst *m,
                  const WaveData::SignalScope *signal_scope) {
        for (const WaveData::Signal &s : signal_scope->signals) {
          s.struct_members.clear();
          // Find the matching net in the design.
          if (m->Nets() == nullptr) continue;
          for (const auto *n : *m->Nets()) {
            if (n->VpiType() == vpiStructNet && n->VpiName() == s.name) {
              const auto *spec = dynamic_cast<const UHDM::struct_typespec *>(
                  n->Typespec()->Actual_typespec());
              ApplyStruct(spec, s, s.width - 1);
              const_cast<WaveData::Signal &>(s).name +=
                  absl::StrFormat("members=%d ", spec->Members()->size());
            }
          }
        }
        for (const WaveData::SignalScope &signal_sub : signal_scope->children) {
          // Find the matching submodule in the design.
          if (m->Modules() == nullptr) continue;
          for (const auto *design_sub : *m->Modules()) {
            if (design_sub->VpiName() == signal_sub.name) {
              apply(design_sub, &signal_sub);
            }
          }
        }
      };
  apply(dynamic_cast<const UHDM::module_inst *>(design_scope), signal_scope);
}
} // namespace sv
