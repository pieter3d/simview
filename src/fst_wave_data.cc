#include "fst_wave_data.h"
#include <stack>
#include <stdexcept>

namespace sv {

FstWaveData::FstWaveData(const std::string &file_name) {
  reader_ = fstReaderOpen(file_name.c_str());
  if (reader_ == nullptr) {
    throw std::runtime_error("Unable to read wave file.");
  }
  std::stack<HierarchyLevel *> stack;
  stack.push(&root_);
  fstHier *h;
  while ((h = fstReaderIterateHier(reader_))) {
    auto &current_level = stack.top();
    switch (h->htyp) {
    case FST_HT_SCOPE: {
      std::string name(h->u.scope.name, h->u.scope.name_length);
      if (stack.size() == 1) {
        // Root node just needs the name.
        root_.name = name;
      } else {
        current_level->children.push_back({});
        current_level->children.back().name = name;
        stack.push(&current_level->children.back());
      }
    } break;
    case FST_HT_UPSCOPE: stack.pop(); break;
    case FST_HT_VAR: {
      current_level->signals.push_back({
          .name = std::string(h->u.var.name, h->u.var.name_length),
          .width = h->u.var.length,
          .id = h->u.var.handle,
      });
      auto &signal = current_level->signals.back();
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

} // namespace sv
