#pragma once

#include "panel.h"
#include "radix.h"
#include "text_input.h"
#include "wave_data.h"

namespace sv {

// The constructor pulls data from the Workspace wave data. Bad things happen if
// there is none.
class WavesPanel : public Panel {
 public:
  WavesPanel();
  void Draw() final;
  void UIChar(int ch) final;
  std::string Tooltip() const final;
  void Resized() final;
  std::optional<std::pair<int, int>> CursorLocation() const final;
  bool Modal() const final { return inputting_time_; }
  int NumLines() const final { return visible_signals_.size(); }
  std::pair<int, int> ScrollArea() const final;
  void AddSignal(const WaveData::Signal *signal);

 private:
  double TimePerChar() const;
  void CycleTimeUnits();
  void UpdateValues();
  void UpdateWaves();
  void DeleteSignal();
  void MoveSignalUp();
  void MoveSignalDown();
  struct ListItem {
    // Helper constructors
    explicit ListItem(const WaveData::Signal *s) : signal(s) {}
    explicit ListItem(const std::string &s) : group_name(s) {}
    explicit ListItem(bool b) : blank(b) {}
    // Helpers
    const std::string &Name() const;
    void CycleRadix();

    // Member data.
    Radix radix = Radix::kHex;
    const WaveData::Signal *signal = nullptr;
    int analog_size = 1;
    bool blank = false;
    // Members when this item represents a group container.
    std::string group_name;
    bool collapsed = false;
    int depth = 0;
  };

  std::vector<ListItem> signals_;
  // Flattened list of all signals, accounting for collapsed groups.
  std::vector<const ListItem *> visible_signals_;
  // Lookup table, mapping a line number in the signal list (with potentially
  // collapsed groups) to indicies in the full signals_ list.
  std::vector<int> visible_to_full_lookup_;
  // Build the flat list and lookup table. Must be called when manipulating the
  // tree.
  void UpdateVisibleSignals();
  // This is used to save the values, since deriving it from the wave database
  // is potentially expensive.
  std::unordered_map<const WaveData::Signal *, std::string> cached_values_;
  std::unordered_map<const WaveData::Signal *, std::vector<WaveData::Sample>>
      cached_waves_;
  int cursor_pos_ = 0;
  uint64_t cursor_time_ = 0;
  uint64_t marker_time_ = 0;
  uint64_t numbered_marker_times_[10];
  uint64_t left_time_ = 0;
  uint64_t right_time_ = 0;
  bool marker_selection_ = false;
  TextInput time_input_;
  TextInput name_input_;
  bool inputting_name_ = false;
  bool inputting_time_ = false;
  int time_unit_ = -9; // nanoseconds.
  int insert_pos_ = 0;

  // Charachters reserved for the signal name and value.
  int name_size_ = 20;
  int value_size_ = 16;
  // Convenience to avoid repeated workspace Get() calls.
  const WaveData *wave_data_;
};

} // namespace sv
