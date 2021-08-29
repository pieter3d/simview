#pragma once

#include "panel.h"
#include "wave_data.h"

namespace sv {

// The constructor pulls data from the Workspace wave data. Bad things happen if
// there is none.
class WavesPanel : public Panel {
 public:
  WavesPanel();
  void Draw() final;
  void UIChar(int ch) final;
  int NumLines() const final { return 0; }
  std::string Tooltip() const final;

 private:
  int cursor_pos_ = 0;
  uint64_t cursor_time_ = 0;
  uint64_t marker_time_ = 0;
  uint64_t numbered_marker_times_[10];
  uint64_t left_time_ = 0;
  uint64_t right_time_ = 0;
  bool marker_selection_ = false;
  // Charachters reserved for the signal name and value.
  int name_size_ = 15;
  int value_size_ = 12;
  // Convenience to avoid repeated workspace Get() calls.
  const WaveData *wave_data_;
  double TimePerChar() const;
};

} // namespace sv
