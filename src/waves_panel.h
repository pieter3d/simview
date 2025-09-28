#pragma once

#include "absl/container/flat_hash_map.h"
#include "panel.h"
#include "radix.h"
#include "text_input.h"
#include "wave_data.h"
#include "wave_image.h"

namespace sv {

// Renders the wave forms in the UI.
// The constructor pulls data from the Workspace wave data. Bad things happen if
// there is none.
class WavesPanel : public Panel {
 public:
  WavesPanel();
  void Draw() final;
  void UIChar(int ch) final;
  std::vector<Tooltip> Tooltips() const final;
  void Resized() final;
  std::optional<std::pair<int, int>> CursorLocation() const final;
  bool Modal() const final;
  int NumLines() const final { return visible_items_.size(); }
  std::pair<int, int> ScrollArea() const final;
  void AddSignal(const WaveData::Signal *signal);
  void AddSignals(const std::vector<const WaveData::Signal *> &signals);
  bool Searchable() const final { return true; }
  bool Search(bool search_down) final;
  std::optional<const WaveData::Signal *> SignalForSource();
  void PrepareForWaveDataReload() final;
  void HandleReloadedWaves() final;
  // Public because the main UI could call this on startup. This allows for wave listing restore on
  // startup via command line specified file.
  void LoadList(const std::string &file_name);

 private:
  struct ListItem {
    // Helper constructors
    explicit ListItem(const WaveData::Signal *s) : signal(s) {}
    explicit ListItem(const std::string &s) : group_name(s) {}
    // Helpers
    std::string Name() const;
    void CycleRadix();
    void CycleAnalogType();
    // Member data.
    Radix radix = Radix::kHex;
    const WaveData::Signal *signal = nullptr;
    int analog_rows = 0;
    AnalogWaveType analog_type = AnalogWaveType::kSampleAndHold;
    int depth = 0;
    int custom_color = -1;
    // Members when this item represents a group container.
    std::string unavailable_name;
    std::string group_name;
    bool expandable_net = false;
    int expanded_bit_idx = -1;
    bool is_group = false;
    bool collapsed = false;
    // Saved here instead of searched and derived every time.
    std::string value;
    int Height() const { return analog_rows > 0 ? analog_rows : 1; }
  };
  double TimePerChar() const;
  void CycleTimeUnits();
  void DeleteItem();
  void MoveSignal(bool up);
  void AddGroup();
  void UpdateValues();
  void UpdateWaves();
  void UpdateValue(ListItem *item);
  void UpdateWave(ListItem *item);
  void SnapToValue();
  void ExpandMultiBit();
  void CheckMultiBit();
  void FindEdge(bool forward, bool *time_changed, bool *range_changed);
  void GoToTime(uint64_t time, bool *time_changed, bool *range_changed);
  void SaveList(const std::string &file_name);
  std::pair<int, int> UIRowsOfLine(int line) const final;

  std::vector<ListItem> items_;
  // Flattened list of all signals, accounting for collapsed groups.
  std::vector<ListItem *> visible_items_;
  // Lookup table, mapping a line number in the signal list (with potentially
  // collapsed groups) to indicies in the full items_ list.
  std::vector<int> visible_to_full_lookup_;
  // Build the flat list and lookup table. Must be called when manipulating the
  // tree.
  void UpdateVisibleSignals();
  // Current time is held by the workspace.
  uint64_t &cursor_time_;
  int cursor_pos_ = 0;
  uint64_t marker_time_ = 0;
  uint64_t numbered_marker_times_[10];
  uint64_t left_time_ = 0;
  uint64_t right_time_ = 0;
  bool marker_selection_ = false;
  bool color_selection_ = false;
  TextInput time_input_;
  TextInput rename_input_;
  TextInput filename_input_;
  ListItem *rename_item_ = nullptr;
  bool inputting_time_ = false;
  bool showing_path_ = false;
  bool inputting_open_ = false;
  bool inputting_save_ = false;
  bool unicode_ = true;
  int time_unit_ = -9; // nanoseconds.
  bool leading_zeroes_ = true;
  const WaveData::Signal *signal_for_source_ = nullptr;

  // Charachters reserved for the signal name and value.
  int name_value_size_ = 30;
  // Convenience to avoid repeated workspace Get() calls.
  const WaveData *wave_data_;

  // When waves are reloaded, the current wave paths are stored here as strings, so they can be
  // re-looked up afterwards.
  absl::flat_hash_map<int, std::string> reload_waves_;
};

} // namespace sv
