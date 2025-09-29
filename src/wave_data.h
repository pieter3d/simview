#pragma once

#include "absl/container/flat_hash_map.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sv {
// The WaveData is an abstract class that holds wave data to be loaded from a
// file. Implementations exist for VCD and FST wave formats.
class WaveData {
 public:
  // Picks the right subclass based on file extension.
  static std::unique_ptr<WaveData> ReadWaveFile(const std::string &file_name, bool keep_glitches);

  struct SignalScope;
  struct Sample {
    uint64_t time;
    std::string value;
  };
  struct SignalStructMember {
    std::string_view name;
    std::vector<SignalStructMember> children; // empty for normal nets.
    int lsb;
    int width;
  };
  struct Signal {
    enum Direction {
      kUnknown,
      kInput,
      kOutput,
      kInout,
      kInternal,
    } direction = Direction::kUnknown;
    // TODO: There are more specific types such as int, logic, wire, string
    // etc. Not sure if it's useful to expose them in the UI.
    enum Type {
      kNet,
      kParameter,
      kStruct,
    } type = Type::kNet;
    std::string name;
    int width;
    int lsb = 0;
    // Set if the signal name string already contains the [msb:lsb] suffix.
    bool has_suffix = false;
    // General purpose identifier. May need to revisit the type since the
    // various wave database formats use different ways to look up signal data.
    uint32_t id;
    // When >= 0, values represent enum labels.
    int enum_id = -1;
    // Containing scope, or parent.
    const SignalScope *scope = nullptr;
    // Modified by design files.
    mutable std::vector<SignalStructMember> struct_members;
    // This can be loaded / reloaded.
    mutable uint64_t valid_start_time = 0;
    mutable uint64_t valid_end_time = 0;
  };
  struct SignalScope {
    std::string name;
    std::vector<SignalScope> children;
    std::vector<Signal> signals;
    const SignalScope *parent = nullptr;
  };
  const std::vector<Sample> &Wave(const Signal *s) const { return waves_[s->id]; }
  const std::vector<SignalScope> &Roots() const { return roots_; }
  std::optional<Signal *> PathToSignal(std::string_view path);
  std::optional<const Signal *> PathToSignal(std::string_view path) const;
  static std::string SignalToPath(const WaveData::Signal *signal);
  // Loads up the waves_ structure with sample data for the given Signal.
  void LoadSignalSamples(const Signal *signal, uint64_t start_time, uint64_t end_time) const;
  // Returns the sample index corresponding to the value at the given time.
  // Search bounds can be constrained to a subset of the wave.
  int FindSampleIndex(uint64_t time, const Signal *signal, int left, int right) const;
  // Variant that searches the whole wave.
  int FindSampleIndex(uint64_t time, const Signal *signal) const;
  // Obtain the textual value of the signal at the given time.
  std::string FindSampleValue(uint64_t time, const Signal *signal) const;
  // Get an enum value if it exists.
  std::optional<std::string_view> GetEnumLabel(int enum_id, std::string_view val) const;

  // ------------- Implementation methods --------------
  // returns -9 for nanoseconds, -6 for microseconds, etc.
  virtual int Log10TimeUnits() const = 0;
  // Valid time range in the wave data.
  virtual std::pair<uint64_t, uint64_t> TimeRange() const = 0;
  // Implementations use a batch processing variant of wave loading, which is
  // generally a lot more efficient than reading the wave data for each signal
  // separately.
  virtual void LoadSignalSamples(const std::vector<const Signal *> &signals, uint64_t start_time,
                                 uint64_t end_time) const = 0;
  virtual void Reload() = 0;

  virtual ~WaveData() {}

 protected:
  // Not directly constructable.
  WaveData(const std::string &file_name, bool keep_glitches)
      : file_name_(file_name), keep_glitches_(keep_glitches) {}
  // Traverse the scopes and signals and assign parents. This can only be done
  // after the structure has been fully created, since the parents are pointers
  // to elements of vectors, and thus could be invalidated (point to garbage) if
  // the signal and scope children vectors are modified.
  void BuildParents();
  // Waveform data is stored per ID, which is potentially a subset of signals
  // in the wave. This avoids the need to hold copies of identical waveforms
  // for signals who are aliases of eachother. The canonical example here is
  // clocks, which have lots of samples and generally exist all throughout the
  // design without differing between scopes.
  // This is marked mutable so that classes that hold a const reference or
  // pointer to this WaveData object can index the map (which is a non-const
  // operation since it may create new empty vectors for new IDs).
  mutable absl::flat_hash_map<uint32_t, std::vector<Sample>> waves_;
  // Signals owned from here.
  std::vector<SignalScope> roots_;
  // File name saved for convenience, for reloads etc.
  std::string file_name_;
  // When false, glitches are stripped from the wave data.
  bool keep_glitches_;
  // Enum value-to-label maps.
  absl::flat_hash_map<int, absl::flat_hash_map<std::string, std::string>> enums_;
};

} // namespace sv
