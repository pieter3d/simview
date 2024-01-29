#include "fst_wave_data.h"
#include "external/fst/fstapi.h"
#include <stack>
#include <stdexcept>

namespace sv {
namespace {

std::string ParseSignalLsb(const std::string &s, int *lsb) {
  auto range_pos = s.find_last_of('[');
  auto colon_pos = s.find_last_of(':');
  if (range_pos != std::string::npos && colon_pos != std::string::npos &&
      range_pos < colon_pos) {
    *lsb = std::stoi(s.substr(colon_pos + 1));
    while (range_pos > 0 && s[range_pos - 1] == ' ') {
      range_pos--;
    }
    return s.substr(0, range_pos + 1);
  } else {
    return s;
  }
}

} // namespace

FstWaveData::FstWaveData(const std::string &file_name, bool keep_glitches)
    : WaveData(file_name, keep_glitches) {
  reader_ = fstReaderOpen(file_name.c_str());
  if (reader_ == nullptr) {
    throw std::runtime_error("Unable to read wave file.");
  }
  ReadScopes();
}

FstWaveData::~FstWaveData() { fstReaderClose(reader_); }

int FstWaveData::Log10TimeUnits() const {
  return fstReaderGetTimescale(reader_);
}

void FstWaveData::ReadScopes() {
  std::stack<SignalScope *> stack;
  fstHier *h;
  while ((h = fstReaderIterateHier(reader_))) {
    switch (h->htyp) {
    case FST_HT_SCOPE: {
      std::string name(h->u.scope.name, h->u.scope.name_length);
      if (stack.empty()) {
        // A root node just needs the name.
        SignalScope root;
        root.name = name;
        roots_.push_back(root);
        // This can be safely done since the roots vector won't be modified
        // until the stack is empty.
        stack.push(&roots_.back());
      } else {
        stack.top()->children.push_back({});
        stack.top()->children.back().name = name;
        stack.push(&stack.top()->children.back());
      }
    } break;
    case FST_HT_UPSCOPE: stack.pop(); break;
    case FST_HT_VAR: {
      std::string name(h->u.var.name, h->u.var.name_length);
      stack.top()->signals.push_back({});
      auto &signal = stack.top()->signals.back();
      signal.id = h->u.var.handle;
      signal.width = h->u.var.length;
      signal.name = ParseSignalLsb(name, &signal.lsb);
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
  BuildParents();
}

std::pair<uint64_t, uint64_t> FstWaveData::TimeRange() const {
  std::pair<uint64_t, uint64_t> range;
  range.first = fstReaderGetStartTime(reader_);
  range.second = fstReaderGetEndTime(reader_);
  return range;
}

void FstWaveData::LoadSignalSamples(const std::vector<const Signal *> &signals,
                                    uint64_t start_time,
                                    uint64_t end_time) const {
  // Build a map to know where each result goes during the unpredictable order
  // in callbacks.
  fstReaderClrFacProcessMaskAll(reader_);
  for (const auto &s : signals) {
    if (s == nullptr) continue;
    // Don't re-read existing waves.
    if (s->valid_start_time <= start_time && s->valid_end_time >= end_time) {
      continue;
    }
    waves_[s->id].clear();
    // Save the time over where the samples are valid.
    s->valid_start_time = start_time;
    s->valid_end_time = end_time;
    // Tell the reader to include this signal while reading the large data
    // blocks.
    fstReaderSetFacProcessMask(reader_, s->id);
  }

  // This is more of a hint, data blocks can read data outside these limits.
  fstReaderSetLimitTimeRange(reader_, start_time, end_time);

  fstReaderIterBlocks(
      reader_,
      +[](void *user_callback_data_pointer, uint64_t time, fstHandle facidx,
          const unsigned char *value) {
        FstWaveData *fst =
            reinterpret_cast<FstWaveData *>(user_callback_data_pointer);
        std::vector<Sample> &samples = fst->waves_[facidx];
        const char *str_val = reinterpret_cast<const char *>(value);
        if (!fst->keep_glitches_ && !samples.empty()) {
          Sample &prev_sample = samples.back();
          if (str_val == prev_sample.value) {
            return; // Ignore duplicates.
          } else if (time == prev_sample.time) {
            // Does this new value make the previous one pointless?
            if (samples.size() > 1 &&
                samples[samples.size() - 2].value == str_val) {
              samples.pop_back();
            } else {
              // Just update the previous with this new value.
              prev_sample.value = str_val;
            }
            return;
          }
        }
        samples.push_back({.time = time, .value = str_val});
      },
      const_cast<FstWaveData *>(this), nullptr);

  // Update the valid range based on sample data actually received.
  for (const auto &s : signals) {
    if (s == nullptr) continue;
    auto &wave = waves_[s->id];
    if (wave.empty()) continue;
    s->valid_start_time = std::min(s->valid_start_time, wave.front().time);
    s->valid_end_time = std::max(s->valid_end_time, wave.back().time);
  }
}

void FstWaveData::Reload() {
  // First, re-create the reader.
  fstReaderClose(reader_);
  reader_ = fstReaderOpen(file_name_.c_str());
  if (reader_ == nullptr) {
    throw std::runtime_error("Unable to read wave file.");
  }
  waves_.clear();
  roots_.clear();
  ReadScopes();
}

} // namespace sv
