#include "fst_wave_data.h"
#include <stack>
#include <stdexcept>

namespace sv {

FstWaveData::FstWaveData(const std::string &file_name) {
  reader_ = fstReaderOpen(file_name.c_str());
  if (reader_ == nullptr) {
    throw std::runtime_error("Unable to read wave file.");
  }
  std::stack<SignalScope *> stack;
  fstHier *h;
  while ((h = fstReaderIterateHier(reader_))) {
    switch (h->htyp) {
    case FST_HT_SCOPE: {
      std::string name(h->u.scope.name, h->u.scope.name_length);
      if (stack.empty()) {
        // Root node just needs the name.
        root_.name = name;
        stack.push(&root_);
      } else {
        stack.top()->children.push_back({});
        stack.top()->children.back().name = name;
        stack.push(&stack.top()->children.back());
      }
    } break;
    case FST_HT_UPSCOPE: stack.pop(); break;
    case FST_HT_VAR: {
      stack.top()->signals.push_back({
          .name = std::string(h->u.var.name, h->u.var.name_length),
          .width = h->u.var.length,
          .id = h->u.var.handle,
      });
      auto &signal = stack.top()->signals.back();
      switch (h->u.var.direction) {
      case FST_VD_IMPLICIT:
        signal.direction = Signal::Direction::kInternal;
        break;
      case FST_VD_INOUT: signal.direction = Signal::Direction::kInout; break;
      case FST_VD_INPUT: signal.direction = Signal::Direction::kInput; break;
      case FST_VD_OUTPUT: signal.direction = Signal::Direction::kOutput; break;
      }
      if (h->u.var.typ == FST_VT_VCD_PARAMETER) {
        signal.type = Signal::Type::kParameter;
      }
    } break;
    }
  }
}

FstWaveData::~FstWaveData() { fstReaderClose(reader_); }

int FstWaveData::Log10TimeUnits() const {
  return fstReaderGetTimescale(reader_);
}

std::pair<uint64_t, uint64_t> FstWaveData::TimeRange() const {
  std::pair<uint64_t, uint64_t> range;
  range.first = fstReaderGetStartTime(reader_);
  range.second = fstReaderGetEndTime(reader_);
  return range;
}

} // namespace sv
