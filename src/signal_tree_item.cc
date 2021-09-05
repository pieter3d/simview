#include "signal_tree_item.h"

namespace sv {
namespace {
std::string kInputString = "->";
std::string kInoutString = "<>";
std::string kOutputString = "<-";
std::string kNetString = "";
std::string kParameterString = "[P]";
} // namespace

const std::string &SignalTreeItem::Name() const { return signal_->name; }

bool SignalTreeItem::AltType() const {
  return signal_->type == WaveData::Signal::kParameter;
}

const std::string &SignalTreeItem::Type() const {
  if (signal_->type == WaveData::Signal::kParameter) {
    return kParameterString;
  }
  switch (signal_->direction) {
  case WaveData::Signal::kInput: return kInputString; break;
  case WaveData::Signal::kOutput: return kOutputString; break;
  case WaveData::Signal::kInout: return kInoutString; break;
  default: return kNetString;
  }
}

} // namespace sv
