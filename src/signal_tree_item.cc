#include "signal_tree_item.h"
#include "absl/strings/str_format.h"

namespace sv {
namespace {
constexpr std::string_view kInputString = "->";
constexpr std::string_view kInoutString = "<>";
constexpr std::string_view kOutputString = "<-";
constexpr std::string_view kNetString;
constexpr std::string_view kParameterString = "[P]";
} // namespace

SignalTreeItem::SignalTreeItem(const WaveData::Signal *s) : signal_(s) {
  if (s->width > 1 && !s->has_suffix) {
    name_ = s->name + absl::StrFormat("[%d:%d]", s->width - 1 + s->lsb, s->lsb);
  } else {
    name_ = s->name;
  }
}

bool SignalTreeItem::AltType() const { return signal_->type == WaveData::Signal::kParameter; }

std::string_view SignalTreeItem::Type() const {
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
