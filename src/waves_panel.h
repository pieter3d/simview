#pragma once

#include "panel.h"
#include "radix.h"
#include "text_input.h"
#include "wave_data.h"
#include <memory>

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
  bool Modal() const final;
  int NumLines() const final { return visible_signals_.size(); }
  std::pair<int, int> ScrollArea() const final;
  void AddSignal(const WaveData::Signal *signal);

 private:
  double TimePerChar() const;
  void DrawHelp() const;
  void CycleTimeUnits();
  void UpdateValues();
  void UpdateWaves();
  void DeleteItem();
  void MoveSignal(bool up);
  void AddGroup();
  struct ListItem {
    // Helper constructors
    explicit ListItem(const WaveData::Signal *s) : signal(s) {}
    explicit ListItem(const std::string &s) : group_name(s) {}
    // Helpers
    const std::string &Name() const;
    void CycleRadix();
    // Member data.
    Radix radix = Radix::kHex;
    const WaveData::Signal *signal = nullptr;
    int analog_size = 1;
    // Members when this item represents a group container.
    std::string group_name;
    bool is_group = false;
    bool collapsed = false;
    int depth = 0;
    std::vector<WaveData::Sample> wave;
  };

  // Owned pointers here makes reordering the contents of the list a lot faster
  // if the user adds groups etc. Otherwise there are copies of large datasets.
  std::vector<std::unique_ptr<ListItem>> signals_;
  // Flattened list of all signals, accounting for collapsed groups.
  std::vector<ListItem *> visible_signals_;
  // Lookup table, mapping a line number in the signal list (with potentially
  // collapsed groups) to indicies in the full signals_ list.
  std::vector<int> visible_to_full_lookup_;
  // Build the flat list and lookup table. Must be called when manipulating the
  // tree.
  void UpdateVisibleSignals();
  int cursor_pos_ = 0;
  uint64_t cursor_time_ = 0;
  uint64_t marker_time_ = 0;
  uint64_t numbered_marker_times_[10];
  uint64_t left_time_ = 0;
  uint64_t right_time_ = 0;
  // Keep track of the time range for which signals have all their wave data.
  uint64_t samples_left_time_ = 0;
  uint64_t samples_right_time_ = 0;
  bool marker_selection_ = false;
  TextInput time_input_;
  TextInput rename_input_;
  ListItem *rename_item_ = nullptr;
  bool inputting_time_ = false;
  bool showing_help_ = false;
  int time_unit_ = -9; // nanoseconds.

  // Charachters reserved for the signal name and value.
  int name_size_ = 20;
  int value_size_ = 16;
  // Convenience to avoid repeated workspace Get() calls.
  const WaveData *wave_data_;
};

} // namespace sv
