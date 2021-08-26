#pragma once

#include "tree_data.h"
#include <memory>

namespace sv {

class WaveData {
 public:
  static std::unique_ptr<WaveData> ReadWaveFile(const std::string &file_name);

  struct Signal {
    enum class Direction {
      kUnknown,
      kInput,
      kOutput,
      kInout,
      kInternal,
    } direction = Direction::kUnknown;
    // TODO: There are more specific types such as int, logic, wire, string
    // etc. Not sure if it's useful to expose them in the UI.
    enum class Type {
      kNet,
      kParameter,
    } type = Type::kNet;
    std::string name;
    uint32_t width;
    // General purpose identifier. May need to revisit the type since the
    // various wave database formats use different ways to look up signal data.
    uint32_t id;
  };
  struct SignalScope {
    std::string name;
    std::vector<SignalScope> children;
    std::vector<Signal> signals;
  };
  virtual ~WaveData() {}
  const SignalScope &Root() const { return root_; }

 protected:
  // Not directly constructable.
  WaveData() = default;
  SignalScope root_;
};

} // namespace sv
