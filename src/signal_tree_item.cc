#include "signal_tree_item.h"

namespace sv {
namespace {
std::string kInputString = "->";
std::string kInoutString = "<>";
std::string kOutputString = "<-";
std::string kNetString = "";
std::string kParameterString = "[P]";
} // namespace

const std::string &SignalTreeItem::Name() const { return signal_.name; }

bool SignalTreeItem::AltType() const {
  return signal_.type == WaveData::Signal::Type::kParameter;
}

const std::string &SignalTreeItem::Type() const {
  if (signal_.type == WaveData::Signal::Type::kParameter) {
    return kParameterString;
  }
  switch (signal_.direction) {
  case WaveData::Signal::Direction::kInput: return kInputString; break;
  case WaveData::Signal::Direction::kOutput: return kOutputString; break;
  case WaveData::Signal::Direction::kInout: return kInoutString; break;
  default: return kNetString;
  }
}

} // namespace sv
