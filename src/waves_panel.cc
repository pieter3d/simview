#include "waves_panel.h"

#include "absl/strings/str_format.h"
#include "color.h"
#include "utils.h"
#include "workspace.h"

namespace sv {

namespace {
constexpr float kZoomStep = 0.9;
constexpr int kSmallestUnit = -18;
constexpr int kMinCharsPerTick = 12;
const char *kTimeUnits[] = {"as", "fs", "ps", "ns", "us",
                            "ms", "s",  "ks", "Ms"};
} // namespace

WavesPanel::WavesPanel() {
  wave_data_ = Workspace::Get().Waves();
  std::tie(left_time_, right_time_) = wave_data_->TimeRange();
  for (int i = 0; i < 10; ++i) {
    numbered_marker_times_[i] = 0;
  }
}

double WavesPanel::TimePerChar() const {
  const int wave_x = name_size_ + value_size_;
  const int wave_width = std::max(1, getmaxx(w_) - wave_x);
  return (right_time_ - left_time_) / (double)wave_width;
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
  int unit_idx = pow10 / 3;
  double factor = pow(10, pow10 % 3);
  while ((right_time_ - left_time_) * factor > 10'000) {
    factor /= 1000;
    unit_idx++;
  }

  // Draw as many ticks as possible, forming the time ruler.
  uint64_t tick_time = left_time_ - (left_time_ % time_per_tick);
  while (1) {
    const int xpos = 0.5 + wave_x + (tick_time - left_time_) / time_per_char;
    const auto s = absl::StrFormat(
        "%s%s", AddDigitSeparators(tick_time * factor), kTimeUnits[unit_idx]);
    tick_time += time_per_tick;
    if (xpos < wave_x) continue;
    if (xpos + s.size() >= max_w) break;
    SetColor(w_, kWavesTimeTickPair);
    mvwaddch(w_, 1, xpos, '|');
    SetColor(w_, kWavesTimeValuePair);
    waddstr(w_, s.c_str());
  }

  // Draw the cursor and markers. These are drawn on top of the ruler.
  // TODO: Don't overwrite the character in the selected wave.
  for (int i = -1; i < 10; ++i) {
    const uint64_t time = i < 0 ? marker_time_ : numbered_marker_times_[i];
    if (time == 0) continue;
    const int marker_pos = 0.5 + (time - left_time_) / time_per_char;
    if (marker_pos >= 0 && marker_pos + wave_x < max_w) {
      std::string marker_label = "m";
      if (i >= 0) marker_label += '0' + i;
      SetColor(w_, kWavesMarkerPair);
      mvwvline(w_, 1, wave_x + marker_pos, ACS_VLINE, max_h - 2);
      if (marker_pos + marker_label.size() < max_w) {
        mvwaddstr(w_, 1, wave_x + marker_pos + 1, marker_label.c_str());
      }
    }
  }
  SetColor(w_, kWavesCursorPair);
  mvwvline(w_, 1, wave_x + cursor_pos_, ACS_VLINE, max_h - 2);

  // Time indicators.
  std::string s =
      absl::StrFormat("time:%s%s ", AddDigitSeparators(cursor_time_ * factor),
                      kTimeUnits[unit_idx]);
  const int marker_start = s.size();
  s +=
      absl::StrFormat("marker:%s%s ", AddDigitSeparators(marker_time_ * factor),
                      kTimeUnits[unit_idx]);
  const int delta_start = s.size();
  s += absl::StrFormat("|t-m|: %s%s",
                       AddDigitSeparators(factor * abs((int64_t)marker_time_ -
                                                       (int64_t)cursor_time_)),
                       kTimeUnits[unit_idx]);
  SetColor(w_, kWavesCursorPair);
  wmove(w_, 0, 0);
  for (int i = 0; i < s.size(); ++i) {
    if (i >= max_w) break;
    if (i == marker_start) SetColor(w_, kWavesMarkerPair);
    if (i == delta_start) SetColor(w_, kWavesDeltaPair);
    waddch(w_, s[i]);
  }

  // TODO: debug stuff.
  //  mvwprintw(w_, 0, 0,
  //            "time_per_tick: %ld, lr: [%ld %ld], time per char: %f"
  //            " n/v: %d %d, marker: %ld",
  //            time_per_tick, left_time_, right_time_, time_per_char,
  //            name_size_, value_size_, marker_time_);
}

void WavesPanel::UIChar(int ch) {
  if (marker_selection_) {
    if (ch >= '0' && ch <= '9') {
      numbered_marker_times_[ch - '0'] = cursor_time_;
    }
    marker_selection_ = false;
  }
  double time_per_char = std::max(1.0, TimePerChar());
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
    int max_cursor_pos = getmaxx(w_) - 1 - (name_size_ + value_size_);
    int max_time = wave_data_->TimeRange().second;
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
    double scale = ch == 'z' ? kZoomStep : (1.0 / kZoomStep);
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
  default: Panel::UIChar(ch);
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
  tt += "g:goto  ";      // TODO
  tt += "eE:edge  ";     // TODO
  tt += "mM:marker  ";   // TODO
  tt += "i:insert  ";    // TODO
  tt += "b:blank  ";     // TODO
  tt += "c:color  ";     // TODO
  tt += "r:radix  ";     // TODO
  tt += "aA:analog  ";   // TODO
  return tt;
}
} // namespace sv
