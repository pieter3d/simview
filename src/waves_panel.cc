#include "waves_panel.h"

#include "absl/strings/str_format.h"
#include "color.h"
#include "utils.h"
#include "workspace.h"
#include <iterator>
#include <memory>
#include <optional>

namespace sv {

namespace {
constexpr float kZoomStep = 0.75;
constexpr int kSmallestUnit = -18;
constexpr int kMinCharsPerTick = 12;
const char *kTimeUnits[] = {"as", "fs", "ps", "ns", "us", "ms", "s", "ks"};

// Binary search for the right sample.
int FindSampleIndex(uint64_t time, const std::vector<WaveData::Sample> &samples,
                    int left, int right) {
  if (right - left <= 1) {
    // Use the new value when on the same time.
    return time < samples[right].time ? left : right;
  }
  const int mid = (left + right) / 2;
  if (time > samples[mid].time) {
    return FindSampleIndex(time, samples, mid, right);
  } else {
    return FindSampleIndex(time, samples, left, mid);
  }
}

} // namespace

WavesPanel::WavesPanel() : time_input_("goto time:"), rename_input_("") {
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
  // Populate the list with a trailing blank signal so that inserts can happen
  // at the end of the list.
  items_.push_back(std::make_unique<ListItem>(nullptr));
  UpdateVisibleSignals();
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

void WavesPanel::Resized() {
  rename_input_.SetDims(line_idx_, visible_items_[line_idx_]->depth,
                        name_size_ - visible_items_[line_idx_]->depth);
  time_input_.SetDims(0, 0, getmaxx(w_));
}

void WavesPanel::SnapToValue() {
  const auto *item = visible_items_[line_idx_];
  if (item->signal == nullptr) return;
  auto time_per_char = TimePerChar();
  // See if there is an edge within the current cursor's character span
  const uint64_t left_time = left_time_ + cursor_pos_ * time_per_char;
  const uint64_t right_time = left_time_ + (cursor_pos_ + 1) * time_per_char;
  const int left_idx =
      FindSampleIndex(left_time, item->wave, 0, item->wave.size() - 1);
  const int right_idx =
      FindSampleIndex(right_time, item->wave, 0, item->wave.size() - 1);
  // Nothing to snap to if there is no transition within this character.
  if (left_idx == right_idx) return;
  // Find transition closest to left edge, but not before.
  int idx = left_idx;
  while (item->wave[idx].time < left_time) {
    idx++;
  }
  // Update the cursor's time to the precise edge.
  cursor_time_ = item->wave[idx].time;
}

std::optional<std::pair<int, int>> WavesPanel::CursorLocation() const {
  if (inputting_time_) return time_input_.CursorPos();
  if (rename_item_ != nullptr) return rename_input_.CursorPos();
  return std::nullopt;
}

std::pair<int, int> WavesPanel::ScrollArea() const {
  // Account for the ruler.
  int h, w;
  getmaxyx(w_, h, w);
  return {h - 1, w};
}

void WavesPanel::UpdateVisibleSignals() {
  visible_items_.clear();
  visible_to_full_lookup_.clear();
  int trim_depth = 0;
  int idx = -1;
  for (auto &item : items_) {
    idx++;
    if (trim_depth != 0 && item->depth >= trim_depth) continue;
    // Stop trimming when reaching an item that is back at legal depth.
    if (item->depth <= trim_depth) trim_depth = 0;
    visible_items_.push_back(item.get());
    visible_to_full_lookup_.push_back(idx);
    // Set a trimming depth if the current item is a collapsed group.
    if (item->is_group && item->collapsed) trim_depth = item->depth + 1;
  }
}

void WavesPanel::Draw() {
  if (showing_help_) {
    DrawHelp();
    return;
  }
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
  wmove(w_, 0, wave_x);
  for (int i = wave_x; i < max_w; ++i) {
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

  // Render signals, values and waves.
  for (int row = 1; row < max_h; ++row) {
    const int list_idx = row - 1 + scroll_row_;
    if (list_idx >= visible_items_.size()) break;
    const auto *item = visible_items_[list_idx];
    const bool highlight =
        multi_line_idx_ < 0
            ? list_idx == line_idx_
            : (list_idx >= std::min(line_idx_, multi_line_idx_) &&
               list_idx <= std::max(line_idx_, multi_line_idx_));
    if (highlight) {
      if (rename_item_ != nullptr) {
        rename_input_.Draw(w_);
        continue;
      } else {
        wattron(w_, has_focus_ ? A_REVERSE : A_UNDERLINE);
      }
    }
    // Treat a blank as a full line of spaces. The -1 accounts for the insert
    // position marker.
    const int len =
        item->signal == nullptr ? (name_size_ - 1) : item->Name().size();

    wmove(w_, row, item->depth);
    if (item->is_group) {
      SetColor(w_, kWavesGroupPair);
      waddch(w_, item->collapsed ? '+' : '-');
      for (int i = 0; i < item->group_name.size(); ++i) {
        // Expander takes up a charachter too.
        const int xpos = item->depth + i + 1;
        if (xpos >= name_size_) break;
        waddch(w_, item->group_name[i]);
      }
    } else {
      SetColor(w_, kWavesSignalNamePair);
      for (int i = 0; i < len; ++i) {
        const int xpos = item->depth + i;
        if (xpos >= name_size_) break;
        // Show an overflow indicator if too narrow.
        if (xpos == name_size_ - 1 && i < len - 1) {
          SetColor(w_, kOverflowTextPair);
          waddch(w_, '>');
        } else {
          waddch(w_, item->signal == nullptr ? ' ' : item->Name()[i]);
        }
      }
    }
    wattrset(w_, A_NORMAL);

    // Render the signal value.
    const int val_start =
        std::max(0, (int)item->value.size() - value_size_ - 1);
    wmove(w_, row,
          std::max(name_size_,
                   name_size_ + value_size_ - 1 - (int)item->value.size()));
    SetColor(w_, kWavesSignalValuePair);
    for (int i = val_start; i < item->value.size(); ++i) {
      if (i == val_start && val_start > 0) {
        SetColor(w_, kOverflowTextPair);
        waddch(w_, '<');
        SetColor(w_, kWavesSignalValuePair);
      } else {
        waddch(w_, item->value[i]);
      }
    }

    // Nothing more to do for blanks/groups.
    if (item->signal == nullptr) continue;

    // Render the signal waveform
    // Single bit signals that aren't too compressed:
    //    ____/"""""""\______
    //
    // Compressed single bit signals shade by duty cycle:
    //
    //   _____|||___|||""""|_||______
    //
    // Multi-bit signals with visible transitions:
    //   =|.5|===16'habcd=====|.123|===16'h0000===
    //
    // Compressed multi-bit signals
    //   ||||=||||||=||||||||||||||||
    //
    // Helper function to set color.
    const bool multi_bit = item->signal->width > 1;
    int left_sample_idx =
        FindSampleIndex(left_time_, item->wave, 0, item->wave.size() - 1);
    // Save the locations of transitions and times to fill in values.
    struct WaveValueInfo {
      int xpos;
      int size;
      uint64_t time;
      std::string value;
    };
    std::vector<WaveValueInfo> wave_value_list;
    // Get the first value ready.
    WaveValueInfo wvi;
    wvi.xpos = 0;
    wvi.time = left_time_;
    int wave_value_idx = left_sample_idx;
    // Determine initial color.
    if (item->wave[left_sample_idx].value.find_first_of("xX") !=
        std::string::npos) {
      SetColor(w_, kWavesXPair + highlight);
    } else if (item->wave[left_sample_idx].value.find_first_of("zZ") !=
               std::string::npos) {
      SetColor(w_, kWavesZPair + highlight);
    } else {
      SetColor(w_, kWavesWaveformPair + highlight);
    }
    // Draw charachter by charachter
    wmove(w_, row, wave_x);
    for (int x = 0; x < max_w - wave_x; ++x) {
      // Find what sample index corresponds to the right edge of this character.
      const int right_sample_idx =
          FindSampleIndex(left_time_ + (1 + x) * time_per_char, item->wave,
                          left_sample_idx, item->wave.size() - 1);
      const int num_transitions = right_sample_idx - left_sample_idx;
      if (num_transitions > 0) {
        // See if anything has X or Z in it. Scan only the new values.
        bool has_x = false;
        bool has_z = false;
        for (int i = left_sample_idx + 1; i <= right_sample_idx; ++i) {
          has_x |= item->wave[i].value.find_first_of("xX") != std::string::npos;
          has_z |= item->wave[i].value.find_first_of("zZ") != std::string::npos;
        }
        // Update color.
        if (has_x)
          SetColor(w_, kWavesXPair + highlight);
        else if (has_z)
          SetColor(w_, kWavesZPair + highlight);
        else
          SetColor(w_, kWavesWaveformPair + highlight);
      }
      if (multi_bit) {
        if (num_transitions == 0) {
          waddch(w_, '=');
        } else {
          waddch(w_, '|');
          // Only save value locations if they are at least 3 characters.
          if (x - wvi.xpos >= 3) {
            wvi.size = x - wvi.xpos;
            wvi.value =
                FormatValue(item->wave[wave_value_idx].value, item->radix,
                            leading_zeroes_, /*drop_size*/ true);
            wave_value_list.push_back(wvi);
          }
          wvi.xpos = x;
          wvi.time = item->wave[right_sample_idx].time;
          wave_value_idx = right_sample_idx;
        }
      } else {
        if (num_transitions == 0) {
          waddch(w_, item->wave[left_sample_idx].value[0] == '0' ? '_' : '"');
        } else if (num_transitions == 1) {
          waddch(w_, item->wave[left_sample_idx].value[0] == '0' ? '/' : '\\');
        } else {
          waddch(w_, '|');
        }
      }
      left_sample_idx = right_sample_idx;
    }
    // Add the remaining wave value if possible, sized against the right edge.
    if (multi_bit && max_w - wave_x - wvi.xpos >= 3) {
      wvi.size = max_w - wave_x - wvi.xpos;
      wvi.value = FormatValue(item->wave[wave_value_idx].value, item->radix,
                              leading_zeroes_, /*drop_size*/ true);
      wave_value_list.push_back(wvi);
    }

    // Draw waveform values inline where possible.
    SetColor(w_, kWavesInlineValuePair + highlight);
    for (const auto &wv : wave_value_list) {
      int start_pos, char_offset;
      if (wv.size - 1 < wv.value.size()) {
        start_pos = wv.xpos + 1;
        char_offset = wv.value.size() - wv.size + 1;
      } else {
        start_pos = wv.xpos + wv.size / 2 - wv.value.size() / 2;
        char_offset = 0;
      }
      wmove(w_, row, start_pos + wave_x);
      for (int i = char_offset; i < wv.value.size(); ++i) {
        waddch(w_, (i == char_offset && char_offset != 0) ? '.' : wv.value[i]);
      }
    }
  }

  // Draw the cursor and markers.
  const auto draw_vline = [&](int row, int col) {
    const int current_line = 1 + line_idx_ - scroll_row_;
    if (visible_items_[line_idx_]->signal != nullptr) {
      // Draw the line in two sections, above and below the current line to
      // avoid overwriting the drawn wave.
      if (current_line > row) {
        mvwvline(w_, row, col, ACS_VLINE, current_line - row);
      }
      if (current_line < max_h - 1) {
        mvwvline(w_, current_line + 1, col, ACS_VLINE,
                 max_h - current_line - 1);
      }
    } else {
      mvwvline(w_, row, col, ACS_VLINE, max_h - row);
    }
  };
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
      draw_vline(row, col);
      if (marker_pos + marker_label.size() < max_w) {
        mvwaddstr(w_, row, col, marker_label.c_str());
      }
    }
  }
  // Also the interactive cursor.
  int cursor_col = wave_x + cursor_pos_;
  int cursor_row = vline_row(cursor_col);
  SetColor(w_, kWavesCursorPair);
  draw_vline(cursor_row, cursor_col);

  // Draw the full path on top of everything else.
  if (showing_path_ && visible_items_[line_idx_]->signal != nullptr) {
    const int ypos = line_idx_ - scroll_row_ + 1;
    SetColor(w_, kWavesCursorPair);
    wmove(w_, ypos, 0);
    std::string path = visible_items_[line_idx_]->Name();
    auto scope = visible_items_[line_idx_]->signal->scope;
    while (scope != nullptr) {
      path = scope->name + "." + path;
      scope = scope->parent;
    }
    for (int x = 0; x < max_w; ++x) {
      if (x >= path.size()) break;
      waddch(w_, path[x]);
    }
  }
}

void WavesPanel::UIChar(int ch) {
  // Convenience.
  double time_per_char = std::max(1.0, TimePerChar());
  bool time_changed = false;
  bool range_changed = false;
  // Most actions cancel multi-line.
  bool cancel_multi_line = true;
  showing_path_ = false; // Anything cancels, but still do other stuff.
  if (showing_help_) {
    showing_help_ = false;
  } else if (marker_selection_) {
    if (ch >= '0' && ch <= '9') {
      numbered_marker_times_[ch - '0'] = cursor_time_;
    }
    marker_selection_ = false;
  } else if (rename_item_ != nullptr) {
    const auto state = rename_input_.HandleKey(ch);
    if (state != TextInput::kTyping) {
      if (state == TextInput::kDone) {
        rename_item_->group_name = rename_input_.Text();
      }
      rename_item_ = nullptr;
      rename_input_.Reset();
    }
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
            time_changed = true;
            const uint64_t current_time_span = right_time_ - left_time_;
            // Ajust left and right bounds if the new time is outside the
            // currently displaye range. Attempt to keep the same zoom scale by
            // keep the time difference between left and right the same.
            if (cursor_time_ > right_time_) {
              right_time_ = cursor_time_;
              left_time_ = right_time_ - current_time_span;
              range_changed = true;
            } else if (cursor_time_ < left_time_) {
              left_time_ = cursor_time_;
              right_time_ = left_time_ + current_time_span;
              range_changed = true;
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
      time_changed = true;
      break;
    case 0x168: // End
    case '$':
      cursor_pos_ = getmaxx(w_) - 1 - (name_size_ + value_size_);
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
      time_changed = true;
      break;
    case 'H':
    case 0x189: // shift-left
    case 'h':
    case 0x104: { // left
      const int step = (ch == 'H' || ch == 0x189) ? 10 : 1;
      if (cursor_pos_ == 0 && left_time_ > 0) {
        left_time_ = std::max(0.0, left_time_ - step * time_per_char);
        right_time_ = std::max(0.0, right_time_ - step * time_per_char);
        range_changed = true;
      } else if (cursor_pos_ > 0) {
        cursor_pos_ = std::max(0, cursor_pos_ - step);
      }
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
      time_changed = true;
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
        range_changed = true;
      } else if (cursor_pos_ < max_cursor_pos) {
        cursor_pos_ = std::min(max_cursor_pos, cursor_pos_ + step);
      }
      cursor_time_ = left_time_ + cursor_pos_ * TimePerChar();
      time_changed = true;
    } break;
    case 0x151: // shift-up
    case 'K':
      if (line_idx_ != 0) {
        if (multi_line_idx_ < 0) multi_line_idx_ = line_idx_;
        Panel::UIChar('k');
        cancel_multi_line = false;
      }
      break;
    case 0x150: // shift-down
    case 'J':
      if (line_idx_ != visible_items_.size() - 1) {
        if (multi_line_idx_ < 0) multi_line_idx_ = line_idx_;
        Panel::UIChar('j');
        cancel_multi_line = false;
      }
      break;
    case 'F': {
      std::tie(left_time_, right_time_) = wave_data_->TimeRange();
      range_changed = true;
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
      range_changed = true;
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
    case 'T':
      time_input_.SetPrompt(absl::StrFormat(
          "Go to time (%s):", kTimeUnits[(time_unit_ - kSmallestUnit) / 3]));
      inputting_time_ = true;
      break;
    case 't': CycleTimeUnits(); break;
    case 0x14a: // Delete
    case 'x': DeleteItem(); break;
    case 'U':
      MoveSignal(true);
      cancel_multi_line = false;
      break;
    case 'D':
      MoveSignal(false);
      cancel_multi_line = false;
      break;
    case 0x7: // Ctrl-g
      AddGroup();
      break;
    case 'R':
      if (visible_items_[line_idx_]->is_group) {
        rename_item_ = visible_items_[line_idx_];
        rename_input_.SetText(visible_items_[line_idx_]->group_name);
      }
      break;
    case 0x20: // space
    case 0xd:  // enter
      if (visible_items_[line_idx_]->is_group) {
        visible_items_[line_idx_]->collapsed =
            !visible_items_[line_idx_]->collapsed;
        UpdateVisibleSignals();
      }
      break;
    case 'r':
      if (visible_items_[line_idx_]->signal != nullptr) {
        visible_items_[line_idx_]->CycleRadix();
        UpdateValue(visible_items_[line_idx_]);
      }
      break;
    case '?': showing_help_ = true; break;
    case 'p': showing_path_ = true; break;
    case 'b': AddSignal(nullptr); break;
    case 'd': signal_for_source_ = visible_items_[line_idx_]->signal; break;
    case '0':
      leading_zeroes_ = !leading_zeroes_;
      UpdateValues();
      break;
    default: Panel::UIChar(ch);
    }
  }
  if (time_changed) {
    SnapToValue();
    UpdateValues();
  }
  if (range_changed) UpdateWaves();
  if (cancel_multi_line) multi_line_idx_ = -1;
}

void WavesPanel::AddSignal(const WaveData::Signal *signal) {
  std::vector<const WaveData::Signal *> one_signal;
  one_signal.push_back(signal);
  AddSignals(one_signal);
}

void WavesPanel::AddSignals(
    const std::vector<const WaveData::Signal *> &signals) {
  // By default, insert at the same depth as the current signal.
  int new_depth = visible_items_[line_idx_]->depth;
  if (line_idx_ > 0 && line_idx_ == items_.size() - 1) {
    // Try to match the depth of the item above when appending at the end.
    auto *prev = visible_items_[line_idx_ - 1];
    // If that item is an uncollapsed group, increase the depth.
    new_depth =
        prev->is_group && !prev->collapsed ? prev->depth + 1 : prev->depth;
  }
  int pos = visible_to_full_lookup_[line_idx_];
  // It's much more efficient to get samples from all signals at once, so no
  // repeated calls to UpdateWave() here.
  const auto waves =
      wave_data_->SignalSamples(signals, left_time_, right_time_);
  int wave_idx = 0;
  for (const auto *signal : signals) {
    items_.insert(items_.begin() + pos, std::make_unique<ListItem>(signal));
    items_[pos]->depth = new_depth;
    items_[pos]->wave = std::move(waves[wave_idx++]);
    UpdateValue(items_[pos].get());
    pos++;
  }
  UpdateVisibleSignals();
  // Move the insert position down, so things generally just nicely append.
  // Only if this isn't a new blank/group.
  if (signals.size() > 1 || signals[0] != nullptr) {
    SetLineAndScroll(line_idx_ + signals.size());
  }
}

void WavesPanel::DeleteItem() {
  // Last line is immutable
  if (line_idx_ == visible_items_.size() - 1) return;
  int start_pos, end_pos;
  start_pos = visible_to_full_lookup_[line_idx_];
  end_pos = multi_line_idx_ < 0 ? start_pos
                                : visible_to_full_lookup_[multi_line_idx_];
  if (start_pos > end_pos) std::swap(start_pos, end_pos);
  // Delete the group and everying under it with greater depth.
  if (items_[end_pos]->is_group) {
    while (end_pos != items_.size() &&
           items_[end_pos + 1]->depth > items_[start_pos]->depth) {
      end_pos++;
    }
  }
  // Preserve the last item.
  if (end_pos == items_.size() - 1) end_pos--;
  items_.erase(items_.begin() + start_pos, items_.begin() + end_pos + 1);
  if (multi_line_idx_ >= 0) line_idx_ = std::min(line_idx_, multi_line_idx_);
  UpdateVisibleSignals();
}

void WavesPanel::MoveSignal(bool up) {
  // Last line is immutable
  if (line_idx_ == visible_items_.size() - 1) return;
  // First line can't move up.
  if (up && (line_idx_ == 0 || multi_line_idx_ == 0)) return;
  auto *item = visible_items_[line_idx_];
  // Second to last line can't move down unless it's not at the lowest depth.
  if (!up &&
      std::max(multi_line_idx_, line_idx_) == visible_items_.size() - 2 &&
      item->depth == 0) {
    return;
  }
  // If a multi-line selection isn't an even depth, just give up. Too
  // complicated.
  if (multi_line_idx_ >= 0) {
    if (visible_items_[line_idx_]->depth !=
        visible_items_[multi_line_idx_]->depth) {
      return;
    }
  }
  // Find the range of things to move.
  int start_pos, end_pos;
  start_pos = visible_to_full_lookup_[line_idx_];
  end_pos = multi_line_idx_ < 0 ? start_pos
                                : visible_to_full_lookup_[multi_line_idx_];
  if (start_pos > end_pos) std::swap(start_pos, end_pos);
  // Move the group and everying under it with greater depth.
  if (items_[end_pos]->is_group) {
    while (end_pos != items_.size() &&
           items_[end_pos + 1]->depth > items_[start_pos]->depth) {
      end_pos++;
    }
  }
  // Preserve the last item.
  if (end_pos == items_.size() - 1) end_pos--;
  // If the last thing to move down is the end of the list, nothing else to do.
  if (!up && end_pos >= items_.size() - 2) return;
  auto adjust_depth = [&](bool deeper) {
    for (int i = start_pos; i <= end_pos; ++i) {
      items_[i]->depth += deeper ? 1 : -1;
    }
  };
  int destination = start_pos;
  if (up) {
    // Move into the above group without moving.
    const int above_pos =
        (multi_line_idx_ < 0 ? line_idx_
                             : std::min(line_idx_, multi_line_idx_)) -
        1;
    const auto *above_item = visible_items_[above_pos];
    if (above_item->depth > item->depth ||
        (above_item->depth == item->depth && above_item->is_group &&
         !above_item->collapsed)) {
      adjust_depth(true);
    } else {
      destination = visible_to_full_lookup_[above_pos];
      if (above_item->depth < item->depth) {
        adjust_depth(false);
      }
    }
  } else {
    // Find the real index of whatever the visible item below the current one
    // is.
    int below_pos = 1 + std::max(multi_line_idx_, line_idx_);
    while (visible_items_[below_pos]->depth > item->depth) {
      below_pos++;
    }
    // If the depth is less, decrease depth without moving.
    if (item->depth > visible_items_[below_pos]->depth) {
      adjust_depth(false);
    } else {
      destination = visible_to_full_lookup_[below_pos];
      // If the below item is a group, move deeper.
      if (visible_items_[below_pos]->is_group &&
          !visible_items_[below_pos]->collapsed) {
        adjust_depth(true);
      }
    }
  }

  if (destination != start_pos) {
    // Create a temporary vector to hold the items to be moved, clear out the
    // old range.
    std::vector<std::unique_ptr<ListItem>> temp_list;
    for (int i = start_pos; i <= end_pos; ++i) {
      temp_list.emplace_back(std::move(items_[i]));
    }
    items_.erase(items_.begin() + start_pos, items_.begin() + end_pos + 1);
    // Insert them at the new spot, accounting for the additional range that was
    // just deleted when moving down.
    const int insert_pos = destination - (up ? 0 : (end_pos - start_pos));
    items_.insert(items_.begin() + insert_pos,
                  std::make_move_iterator(temp_list.begin()),
                  std::make_move_iterator(temp_list.end()));
    // Also move the selected line down.
    line_idx_ += up ? -1 : 1;
    if (multi_line_idx_ >= 0) multi_line_idx_ += up ? -1 : 1;
  }
  UpdateVisibleSignals();
}

void WavesPanel::UpdateWave(ListItem *item) {
  auto samples =
      wave_data_->SignalSamples(item->signal, left_time_, right_time_);
  item->wave = std::move(samples);
}

void WavesPanel::UpdateValue(ListItem *item) {
  if (item->is_group || item->signal == nullptr) return;
  if (item->wave.empty()) {
    item->value = "Unavailable";
    return;
  }
  const uint64_t idx =
      FindSampleIndex(cursor_time_, item->wave, 0, item->wave.size() - 1);
  item->value =
      FormatValue(item->wave[idx].value, item->radix, leading_zeroes_);
}

void WavesPanel::UpdateValues() {
  for (auto *item : visible_items_) {
    UpdateValue(item);
  }
}

void WavesPanel::UpdateWaves() {
  std::vector<const WaveData::Signal *> signal_list;
  std::vector<ListItem *> items_to_update;
  for (auto *item : visible_items_) {
    if (item->signal == nullptr) continue;
    // Skip the update if all the data is already present.
    if (!item->wave.empty() && item->wave.front().time >= left_time_ &&
        item->wave.back().time <= right_time_) {
      continue;
    }
    signal_list.push_back(item->signal);
    items_to_update.push_back(item);
  }
  auto waves = wave_data_->SignalSamples(signal_list, left_time_, right_time_);
  // The samples return is a vector of vector of samples, in the same order as
  // the input list.
  int wave_idx = 0;
  for (auto *item : items_to_update) {
    item->wave = std::move(waves[wave_idx++]);
  }
}

void WavesPanel::AddGroup() {
  // Insert at the top of a multi-line selection. The swap would screw up a
  // multi line selection process, but adding a group anyway cancels the
  // multi-line process.
  if (multi_line_idx_ >= 0 && multi_line_idx_ < line_idx_) {
    std::swap(line_idx_, multi_line_idx_);
  }
  AddSignal(nullptr);
  // Mark the newly added item as needing rename.
  rename_item_ = visible_items_[line_idx_];
  rename_item_->is_group = true;
  rename_input_.SetDims(line_idx_ + 1 - scroll_row_, rename_item_->depth,
                        name_size_ - rename_item_->depth);
  rename_input_.SetText("");
  if (multi_line_idx_ < 0) return;
  // If mutliple lines were selected and they are all the same depth, move them
  // under the group.
  const int start_pos = line_idx_ + 1;
  // Avoid groupifying the last item.
  const int end_pos =
      std::min(multi_line_idx_ + 1, (int)visible_items_.size() - 2);
  for (int i = start_pos; i <= end_pos; ++i) {
    if (visible_items_[i]->depth != visible_items_[line_idx_]->depth) return;
  }
  for (int i = visible_to_full_lookup_[start_pos];
       i <= visible_to_full_lookup_[end_pos]; ++i) {
    items_[i]->depth++;
  }
}

bool WavesPanel::Modal() const {
  return rename_item_ != nullptr || inputting_time_ || showing_help_ ||
         showing_path_;
}

std::string WavesPanel::Tooltip() const {
  if (marker_selection_) {
    return "0-9:marker selection";
  }
  return "?:show help  ";
}

void WavesPanel::DrawHelp() const {
  std::vector<std::string> keys({
      "zZ:  Zoom about the cursor",
      "F:   Zoom full range",
      "sS:  Adjust signal name size",
      "vV:  Adjust value size",
      "0:   Show leading zeroes",
      "p:   Show full signal path",
      "T:   Go to time",
      "t:   Cycle time units",
      "eE:  Previous / next signal edge",
      "m:   Place marker",
      "M:   Place numbered marker",
      "C-g: Create group",
      "R:   Rename group",
      "UD:  Move signal up / down",
      "b:   Insert blank signal",
      "x:   Delete highlighted signal",
      "c:   Change signal color",
      "r:   Cycle signal radix",
      "aA:  Adjust analog signal height",
      "d:   Show signal declaration in source",
  });
  // Find the widest signal.
  int widest_text = 0;
  for (auto s : keys) {
    widest_text = std::max(widest_text, (int)s.size());
  }
  widest_text += 2; // margin around the help box.
  const int max_w = getmaxx(w_);
  const int max_h = getmaxy(w_);
  const int x_start = std::max(0, max_w / 2 - widest_text / 2);
  const int x_stop = std::min(max_w - 1, max_w / 2 + widest_text / 2);
  const int y_start = std::max(0, max_h / 2 - (int)keys.size() / 2);
  const int y_stop = std::min(max_h - 1, max_h / 2 + (int)keys.size() / 2);
  for (int y = 0; y <= std::min((int)keys.size() - 1, y_stop - y_start); ++y) {
    wmove(w_, y_start + y, x_start);
    SetColor(w_, kTooltipPair);
    waddch(w_, ' ');
    SetColor(w_, kTooltipKeyPair);
    const auto &s = keys[y];
    for (int x = 0; x <= x_stop - x_start; ++x) {
      if (s[x] == ':') SetColor(w_, kTooltipPair);
      waddch(w_, x < s.size() ? s[x] : ' ');
    }
  }
}

const std::string &WavesPanel::ListItem::Name() const {
  static std::string blank_string;
  if (is_group) return group_name;
  if (signal == nullptr) return blank_string;
  return signal->name_width;
}

void WavesPanel::ListItem::CycleRadix() {
  switch (radix) {
  case Radix::kHex: radix = Radix::kBinary; break;
  case Radix::kBinary:
    // Don't bother with decimal or float on huge values.
    radix = signal->width > 64 ? Radix::kHex : Radix::kSignedDecimal;
    break;
  case Radix::kSignedDecimal: radix = Radix::kUnsignedDecimal; break;
  case Radix::kUnsignedDecimal:
    // Float only makes sense for fp32/fp64 formats.
    radix = signal->width == 32 || signal->width == 64 ? Radix::kFloat
                                                       : Radix::kHex;
    break;
  case Radix::kFloat: radix = Radix::kHex; break;
  }
}

std::optional<const WaveData::Signal *> WavesPanel::SignalForSource() {
  if (signal_for_source_ == nullptr) return std::nullopt;
  auto s = signal_for_source_;
  signal_for_source_ = nullptr;
  return s;
}

bool WavesPanel::Search(bool search_down) {
  int idx = line_idx_;
  const int start_idx = idx;
  const auto step = [&] {
    idx += search_down ? 1 : -1;
    if (idx < 0) {
      idx = items_.size() - 1;
    } else if (idx >= items_.size()) {
      idx = 0;
    }
  };
  while (1) {
    if (!search_preview_) step();
    const auto pos = items_[idx]->Name().find(search_text_, 0);
    if (pos != std::string::npos) {
      search_start_col_ = pos;
      SetLineAndScroll(idx);
      return true;
    }
    if (search_preview_) step();
    if (idx == start_idx) {
      search_start_col_ = -1;
      return false;
    }
  }
}
} // namespace sv
