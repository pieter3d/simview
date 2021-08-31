#include "waves_panel.h"

#include "absl/strings/str_format.h"
#include "color.h"
#include "utils.h"
#include "workspace.h"
#include <optional>

namespace sv {

namespace {
constexpr float kZoomStep = 0.75;
constexpr int kSmallestUnit = -18;
constexpr int kMinCharsPerTick = 12;
const char *kTimeUnits[] = {"as", "fs", "ps", "ns", "us", "ms", "s", "ks"};
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
  signals_.push_back(ListItem(nullptr));
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
  rename_input_.SetDims(line_idx_, visible_signals_[line_idx_]->depth,
                        name_size_);
  time_input_.SetDims(0, 0, getmaxx(w_));
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
  visible_signals_.clear();
  visible_to_full_lookup_.clear();
  int trim_depth = 0;
  int idx = -1;
  for (auto &item : signals_) {
    idx++;
    if (trim_depth != 0 && item.depth >= trim_depth) continue;
    // Stop trimming when reaching an item that is back at legal depth.
    if (item.depth <= trim_depth) trim_depth = 0;
    visible_signals_.push_back(&item);
    visible_to_full_lookup_.push_back(idx);
    // Set a trimming depth if the current item is a collapsed group.
    if (item.is_group && item.collapsed) trim_depth = item.depth + 1;
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

  // Render signal name list. Using the the TreePanel data here since it has the
  // flattened list with proper tree state etc.
  for (int row = 1; row < max_h; ++row) {
    const int list_idx = row - 1 + scroll_row_;
    if (list_idx >= visible_signals_.size()) break;
    const auto *item = visible_signals_[list_idx];
    if (list_idx == line_idx_) {
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
  }
}

void WavesPanel::UIChar(int ch) {
  // Convenience.
  double time_per_char = std::max(1.0, TimePerChar());
  bool time_changed = false;
  bool range_changed = false;
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
    case 0x7: // Ctrl-g
      AddGroup();
      break;
    case 'r':
      if (visible_signals_[line_idx_]->is_group) {
        rename_item_ = visible_signals_[line_idx_];
        rename_input_.SetText(visible_signals_[line_idx_]->group_name);
      }
      break;
    case 0x20: // space
    case 0xd:  // enter
      if (visible_signals_[line_idx_]->is_group) {
        visible_signals_[line_idx_]->collapsed =
            !visible_signals_[line_idx_]->collapsed;
        UpdateVisibleSignals();
      }
      break;
    case '?': showing_help_ = true; break;
    case 'b': AddSignal(nullptr); break;
    default: Panel::UIChar(ch);
    }
  }
  if (time_changed) UpdateValues();
  if (range_changed) UpdateWaves();
}

void WavesPanel::AddSignal(const WaveData::Signal *signal) {
  // By default, insert at the same depth as the current signal.
  int new_depth = visible_signals_[line_idx_]->depth;
  if (line_idx_ > 0) {
    // Try to match the depth of the item above.
    auto *prev = visible_signals_[line_idx_ - 1];
    // If that item is an uncollapsed group, increase the depth.
    new_depth =
        prev->is_group && !prev->collapsed ? prev->depth + 1 : prev->depth;
  }
  const int pos = visible_to_full_lookup_[line_idx_];
  signals_.insert(signals_.begin() + pos, ListItem(signal));
  signals_[pos].depth = new_depth;
  UpdateVisibleSignals();
  // Move the insert position down, so things generally just nicely append.
  // Only if this isn't a new blank/group.
  if (signal != nullptr) {
    SetLineAndScroll(line_idx_ + 1);
  }
}

void WavesPanel::DeleteItem() {
  // Last line is immutable
  if (line_idx_ == visible_signals_.size() - 1) return;
}

void WavesPanel::MoveSignalUp() {
  // Last line is immutable
  if (line_idx_ == visible_signals_.size() - 1) return;
}

void WavesPanel::MoveSignalDown() {
  // Last line is immutable. Don't move into it.
  if (line_idx_ >= visible_signals_.size() - 2) return;
}

void WavesPanel::UpdateValues() {}

void WavesPanel::UpdateWaves() {}

void WavesPanel::AddGroup() {
  AddSignal(nullptr);
  // Mark the newly added item as needing rename.
  rename_item_ = visible_signals_[line_idx_];
  rename_item_->is_group = true;
  rename_input_.SetDims(line_idx_ + 1 - scroll_row_, rename_item_->depth,
                        name_size_);
  rename_input_.SetText("NewGroup");
}

bool WavesPanel::Modal() const {
  return rename_item_ != nullptr || inputting_time_ || showing_help_;
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
      "p:   Show full signal path",
      "T:   Go to time",
      "t:   Cycle time units",
      "eE:  Previous / next signal edge",
      "m:   Place marker",
      "M:   Place numbered marker",
      "C-g: Create group",
      "r:   Rename group",
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
  return signal->name;
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
} // namespace sv
