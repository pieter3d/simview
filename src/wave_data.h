#pragma once

#include <memory>
#include <vector>

namespace sv {

class WaveData {
 public:
  static std::unique_ptr<WaveData> ReadWaveFile(const std::string &file_name);

  struct SignalScope;
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
    } type = Type::kNet;
    std::string name;
    uint32_t width;
    // General purpose identifier. May need to revisit the type since the
    // various wave database formats use different ways to look up signal data.
    uint32_t id;
    // Containing scope, or parent.
    SignalScope *scope;
  };
  struct SignalScope {
    std::string name;
    std::vector<SignalScope> children;
    std::vector<Signal> signals;
    SignalScope *parent;
  };
  virtual ~WaveData() {}
  // returns -9 for nanoseconds, -6 for microseconds, etc.
  virtual int Log10TimeUnits() const = 0;
  // Valid time range in the wave data.
  virtual std::pair<uint64_t, uint64_t> TimeRange() const = 0;
  // Returns signal data as binary string of digits 01xz
  virtual std::string SignalValue(const Signal *s, uint64_t time) const = 0;
  struct Sample {
    uint64_t time;
    std::string value;
  };
  virtual std::vector<Sample> SignalSamples(const Signal *signal,
                                            uint64_t start_time,
                                            uint64_t end_time) const = 0;
  // Batch processing variant, which is often significantly more efficient than
  // iterating over a set of signal separately.
  virtual std::vector<std::vector<Sample>>
  SignalSamples(const std::vector<const Signal *> &signals, uint64_t start_time,
                uint64_t end_time) const = 0;
  const SignalScope &Root() const { return root_; }

 protected:
  // Not directly constructable.
  WaveData() = default;
  SignalScope root_;
};

} // namespace sv
