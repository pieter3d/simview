#include "waves_panel.h"

#include "absl/strings/str_format.h"
#include "color.h"
#include "utils.h"
#include "workspace.h"

namespace sv {

namespace {
constexpr float kZoomStep = 0.75;
constexpr int kSmallestUnit = -18;
constexpr int kMinCharsPerTick = 12;
const char *kTimeUnits[] = {"as", "fs", "ps", "ns", "us", "ms", "s", "ks"};
} // namespace

WavesPanel::WavesPanel() : time_input_("goto time:") {
  wave_data_ = Workspace::Get().Waves();
  std::tie(left_time_, right_time_) = wave_data_->TimeRange();
  for (int i = 0; i < 10; ++i) {
    numbered_marker_times_[i] = 0;
  }
  time_unit_ = wave_data_->Log10TimeUnits();
  time_unit_ -= time_unit_ % 3; // Align to an SI unit.
  time_input_.SetDims(1, 0, getmaxx(w_));
  time_input_.SetValdiator([&](const std::string &s) {
    auto parsed = ParseTime(s, time_unit_);
    if (!parsed) return false;
    const uint64_t new_time = *parsed;
    return new_time >= wave_data_->TimeRange().first &&
           new_time <= wave_data_->TimeRange().second;
  });
}

void WavesPanel::CycleTimeUnits() {
  int smallest_unit = wave_data_->Log10TimeUnits();
  int max_time = wave_data_->TimeRange().second;
  int max_steps = 0;
  while (max_time > 10'000) {
    max_steps++;
    max_time /= 1000;
  }
  time_unit_ += 3;
  if (time_unit_ > smallest_unit + max_steps * 3) {
    time_unit_ = smallest_unit;
  }
}

double WavesPanel::TimePerChar() const {
  const int wave_x = name_size_ + value_size_;
  const int wave_width = std::max(1, getmaxx(w_) - wave_x);
  return (right_time_ - left_time_) / (double)wave_width;
}

void WavesPanel::Resized() { time_input_.SetDims(0, 0, getmaxx(w_)); }

std::optional<std::pair<int, int>> WavesPanel::CursorLocation() const {
  if (!inputting_time_) return std::nullopt;
  return time_input_.CursorPos();
}

void WavesPanel::Draw() {
  werase(w_);
  const int wave_x = name_size_ + value_size_;
  const int max_w = getmaxx(w_);
  const int max_h = getmaxy(w_);
  const double time_per_char = TimePerChar();
  // Find a multipler that has reasonable spacing.
  // Try factors of 2, 5, 10.
  uint64_t time_per_tick = 1;
  int phase = 0;
  while (time_per_tick / time_per_char < kMinCharsPerTick) {
    switch (phase) {
    case 0: time_per_tick *= 2; break;
    case 1: time_per_tick = (time_per_tick / 2) * 5; break;
    case 2: time_per_tick *= 2; break;
    }
    phase = (phase + 1) % 3;
  }

  // Try to pick a nice unit for the full range.
  int pow10 = (wave_data_->Log10TimeUnits() - kSmallestUnit);
  int ruler_unit_idx = pow10 / 3;
  double ruler_factor = pow(10, pow10 % 3);
  while ((right_time_ - left_time_) * ruler_factor > 10'000) {
    ruler_factor /= 1000;
    ruler_unit_idx++;
  }

  // Fill the ruler with dots first.
  SetColor(w_, kWavesTimeTickPair);
  wmove(w_, 0, 0);
  for (int i = 0; i < max_w; ++i) {
    waddch(w_, '.');
  }
  // Draw as many ticks as possible, forming the time ruler.
  uint64_t tick_time = left_time_ - (left_time_ % time_per_tick);
  while (1) {
    const int xpos = 0.5 + wave_x + (tick_time - left_time_) / time_per_char;
    const auto s =
        absl::StrFormat("%s%s", AddDigitSeparators(tick_time * ruler_factor),
                        kTimeUnits[ruler_unit_idx]);
    tick_time += time_per_tick;
    if (xpos < wave_x) continue;
    if (xpos + s.size() >= max_w) break;
    SetColor(w_, kWavesTimeTickPair);
    mvwaddch(w_, 0, xpos, '|');
    SetColor(w_, kWavesTimeValuePair);
    waddstr(w_, s.c_str());
  }

  // Time indicators.
  int time_width = max_w;
  if (inputting_time_) {
    time_input_.Draw(w_);
  } else {
    const char *unit_string = kTimeUnits[(time_unit_ - kSmallestUnit) / 3];
    double time_factor = pow(10, wave_data_->Log10TimeUnits() - time_unit_);
    std::string s = absl::StrFormat(
        "time:%s%s ", AddDigitSeparators(cursor_time_ * time_factor),
        unit_string);
    const int marker_start = s.size();
    s += absl::StrFormat("marker:%s%s ",
                         AddDigitSeparators(marker_time_ * time_factor),
                         unit_string);
    const int delta_start = s.size();
    s += absl::StrFormat(
        "|t-m|:%s%s ",
        AddDigitSeparators(time_factor *
                           abs((int64_t)marker_time_ - (int64_t)cursor_time_)),
        unit_string);
    time_width = s.size();
    SetColor(w_, kWavesCursorPair);
    wmove(w_, 0, 0);
    for (int i = 0; i < s.size(); ++i) {
      if (i >= max_w) break;
      if (i == marker_start) SetColor(w_, kWavesMarkerPair);
      if (i == delta_start) SetColor(w_, kWavesDeltaPair);
      waddch(w_, s[i]);
    }
  }

  // Draw the cursor and markers. These are drawn on top of the ruler, but not
  // over the time indicators.
  // TODO: Don't overwrite the character in the selected wave.
  auto vline_row = [&](int col) { return col >= time_width ? 0 : 1; };
  for (int i = -1; i < 10; ++i) {
    const uint64_t time = i < 0 ? marker_time_ : numbered_marker_times_[i];
    if (time == 0) continue;
    const int marker_pos = 0.5 + (time - left_time_) / time_per_char;
    if (marker_pos >= 0 && marker_pos + wave_x < max_w) {
      std::string marker_label = "m";
      if (i >= 0) marker_label += '0' + i;
      SetColor(w_, kWavesMarkerPair);
      const int col = wave_x + marker_pos;
      const int row = vline_row(wave_x + marker_pos);
      mvwvline(w_, row, col, ACS_VLINE, max_h - row);
      if (marker_pos + marker_label.size() < max_w) {
        mvwaddstr(w_, row, col, marker_label.c_str());
      }
    }
  }
  // Also the interactive cursor.
  int cursor_col = wave_x + cursor_pos_;
  int cursor_row = vline_row(cursor_col);
  SetColor(w_, kWavesCursorPair);
  mvwvline(w_, cursor_row, cursor_col, ACS_VLINE, max_h - cursor_row);

  // TODO: debug stuff.
  //  mvwprintw(w_, 0, 0,
  //            "time_per_tick: %ld, lr: [%ld %ld], time per char: %f"
  //            " n/v: %d %d, marker: %ld",
  //            time_per_tick, left_time_, right_time_, time_per_char,
  //            name_size_, value_size_, marker_time_);
}

void WavesPanel::UIChar(int ch) {
  // Convenience.
  double time_per_char = std::max(1.0, TimePerChar());
  if (marker_selection_) {
    if (ch >= '0' && ch <= '9') {
      numbered_marker_times_[ch - '0'] = cursor_time_;
    }
    marker_selection_ = false;
  } else if (inputting_time_) {
    const auto state = time_input_.HandleKey(ch);
    if (state != TextInput::kTyping) {
      inputting_time_ = false;
      if (state == TextInput::kDone) {
        if (auto parsed = ParseTime(time_input_.Text(), time_unit_)) {
          const uint64_t new_time = *parsed;
          // Make sure the parsed time actually is inside the wave data.
          if (new_time >= wave_data_->TimeRange().first &&
              new_time <= wave_data_->TimeRange().second) {
            cursor_time_ = new_time;
            const uint64_t current_time_span = right_time_ - left_time_;
            // Ajust left and right bounds if the new time is outside the
            // currently displaye range. Attempt to keep the same zoom scale by
            // keep the time difference between left and right the same.
            if (cursor_time_ > right_time_) {
              right_time_ = cursor_time_;
              left_time_ = right_time_ - current_time_span;
            } else if (cursor_time_ < left_time_) {
              left_time_ = cursor_time_;
              right_time_ = left_time_ + current_time_span;
            }
            // Don't let the cursor go off the screen.
            const int max_cursor_pos =
                getmaxx(w_) - 1 - (name_size_ + value_size_);
            cursor_pos_ = std::min((double)max_cursor_pos,
                                   (cursor_time_ - left_time_) / time_per_char);
          }
        }
      }
      time_input_.Reset();
    }
  } else {
    switch (ch) {
    case 0x106: // Home
    case '^':
      cursor_pos_ = 0;
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
      break;
    case 0x168: // End
    case '$':
      cursor_pos_ = getmaxx(w_) - 1 - (name_size_ + value_size_);
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
      break;
    case 'H':
    case 0x189: // shift-left
    case 'h':
    case 0x104: { // left
      const int step = (ch == 'H' || ch == 0x189) ? 10 : 1;
      if (cursor_pos_ == 0 && left_time_ > 0) {
        left_time_ = std::max(0.0, left_time_ - step * time_per_char);
        right_time_ = std::max(0.0, right_time_ - step * time_per_char);
      } else if (cursor_pos_ > 0) {
        cursor_pos_ = std::max(0, cursor_pos_ - step);
      }
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
    } break;
    case 'L':
    case 0x192: // shift-right
    case 'l':
    case 0x105: { // right
      const int step = (ch == 'L' || ch == 0x192) ? 10 : 1;
      const int max_cursor_pos = getmaxx(w_) - 1 - (name_size_ + value_size_);
      const int max_time = wave_data_->TimeRange().second;
      if (cursor_pos_ == max_cursor_pos && right_time_ < max_time) {
        left_time_ =
            std::min((double)max_time, left_time_ + step * time_per_char);
        right_time_ =
            std::min((double)max_time, right_time_ + step * time_per_char);
      } else if (cursor_pos_ < max_cursor_pos) {
        cursor_pos_ = std::min(max_cursor_pos, cursor_pos_ + step);
      }
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
    } break;
    case 'F': {
      std::tie(left_time_, right_time_) = wave_data_->TimeRange();
    } break;
    case 'z':
    case 'Z': {
      // No point in going further than this.
      const double scale = ch == 'z' ? kZoomStep : (1.0 / kZoomStep);
      if (scale < 1 && right_time_ - left_time_ < 10) break;
      left_time_ =
          std::max(0.0, cursor_time_ - scale * (cursor_time_ - left_time_));
      right_time_ = std::min(
          wave_data_->TimeRange().second,
          (uint64_t)(cursor_time_ + scale * (right_time_ - cursor_time_)));
    } break;
    case 's':
      if (name_size_ + value_size_ < getmaxx(w_) - 20) name_size_++;
      break;
    case 'S':
      if (name_size_ > 5) name_size_--;
      break;
    case 'v':
      if (name_size_ + value_size_ < getmaxx(w_) - 20) value_size_++;
      break;
    case 'V':
      if (value_size_ > 5) value_size_--;
      break;
    case 'm': marker_time_ = cursor_time_; break;
    case 'M': marker_selection_ = true; break;
    case 'g':
      time_input_.SetPrompt(absl::StrFormat(
          "Go to time (%s):", kTimeUnits[(time_unit_ - kSmallestUnit) / 3]));
      inputting_time_ = true;
      break;
    case 't': CycleTimeUnits(); break;
    default: Panel::UIChar(ch);
    }
  }
}

std::string WavesPanel::Tooltip() const {
  std::string tt;
  if (marker_selection_) {
    return "0-9:marker selection";
  }
  if (Workspace::Get().Design() != nullptr) tt += "s:source  "; // TODO
  tt += "F:zoom full  ";
  tt += "zZ:zoom  ";
  tt += "sS:signals  ";
  tt += "vV:values  ";
  tt += "h:hierarchy  "; // TODO
  tt += "g:goto  ";
  tt += "t:units  ";
  tt += "eE:edge  "; // TODO
  tt += "mM:marker  ";
  tt += "i:insert  ";   // TODO
  tt += "ud:up/down  "; // TODO
  tt += "b:blank  ";    // TODO
  tt += "c:color  ";    // TODO
  tt += "r:radix  ";    // TODO
  tt += "aA:analog  ";  // TODO
  return tt;
}
} // namespace sv
