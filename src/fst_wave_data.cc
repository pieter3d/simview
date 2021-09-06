#include "fst_wave_data.h"
#include "absl/strings/str_format.h"
#include <stack>
#include <stdexcept>
#include <unordered_map>

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
      std::string name(h->u.var.name, h->u.var.name_length);
      std::string width = h->u.var.length > 1
                              ? absl::StrFormat("[%d:0]", h->u.var.length - 1)
                              : "";
      stack.top()->signals.push_back({
          .name = name,
          .name_width = name + width,
          .width = h->u.var.length,
          .id = h->u.var.handle,
      });
      auto &signal = stack.top()->signals.back();
      switch (h->u.var.direction) {
      case FST_VD_IMPLICIT: signal.direction = Signal::kInternal; break;
      case FST_VD_INOUT: signal.direction = Signal::kInout; break;
      case FST_VD_INPUT: signal.direction = Signal::kInput; break;
      case FST_VD_OUTPUT: signal.direction = Signal::kOutput; break;
      }
      if (h->u.var.typ == FST_VT_VCD_PARAMETER) {
        signal.type = Signal::kParameter;
      }
    } break;
    }
  }
  // Now that everything has been read and allocated, recurse the tree and fill
  // in parents. Can't do this on the fly since vectors get re-allocated and
  // shuffled around, so pointers derived there won't necessarily be correct
  // after wave data reading is finished.
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
  recurse_assign_parents(&root_);
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

std::string FstWaveData::SignalValue(const Signal *signal,
                                     uint64_t time) const {
  // Allocated space for a null terminated string is required
  std::string val(signal->width + 1, '\0');
  fstReaderGetValueFromHandleAtTime(reader_, time, signal->id, val.data());
  return val;
}

std::vector<WaveData::Sample>
FstWaveData::SignalSamples(const Signal *signal, uint64_t start_time,
                           uint64_t end_time) const {
  // Use the batch version.
  std::vector<const Signal *> sigs({signal});
  return SignalSamples(sigs, start_time, end_time)[0];
}

std::vector<std::vector<WaveData::Sample>>
FstWaveData::SignalSamples(const std::vector<const Signal *> &signals,
                           uint64_t start_time, uint64_t end_time) const {
  std::vector<std::vector<Sample>> samples(signals.size());
  // Build a map to know where each result goes during the unpredictable order
  // in callbacks.
  std::unordered_map<uint32_t, uint32_t> id_map;
  fstReaderClrFacProcessMaskAll(reader_);
  int signal_index = 0;
  for (const auto &s : signals) {
    if (s == nullptr) continue;
    fstReaderSetFacProcessMask(reader_, s->id);
    id_map[s->id] = signal_index++;
  }
  fstReaderSetLimitTimeRange(reader_, start_time, end_time);
  // Wrap the locals in an object that can be passed to the callback.
  struct {
    decltype(id_map) *wrapped_id_map;
    decltype(samples) *wrapped_samples;
    uint64_t wrapped_start_time;
    uint64_t wrapped_end_time;
  } locals;
  locals.wrapped_id_map = &id_map;
  locals.wrapped_samples = &samples;
  locals.wrapped_start_time = start_time;
  locals.wrapped_end_time = end_time;

  fstReaderIterBlocks(
      reader_,
      +[](void *user_callback_data_pointer, uint64_t time, fstHandle facidx,
          const unsigned char *value) {
        const auto vars =
            reinterpret_cast<decltype(locals) *>(user_callback_data_pointer);
        (*vars->wrapped_samples)[(*vars->wrapped_id_map)[facidx]].push_back(
            {.time = time, .value = reinterpret_cast<const char *>(value)});
      },
      &locals, nullptr);
  return samples;
}

} // namespace sv
