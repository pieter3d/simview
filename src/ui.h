#pragma once

#include "absl/time/time.h"
#include "design_tree_panel.h"
#include "source_panel.h"
#include "text_input.h"
#include "wave_signals_panel.h"
#include "wavedata_tree_panel.h"
#include "waves_panel.h"
#include <memory>

namespace sv {

class UI {
 public:
  UI();
  ~UI();

  void EventLoop();

 private:
  void CalcLayout(bool update_frac = false);
  void LayoutPanels();
  void Draw();
  void CycleFocus(bool fwd);

  std::vector<int> tmp_ch; // TODO: remove
  absl::Time last_ch;

  std::unique_ptr<DesignTreePanel> design_tree_panel_;
  std::unique_ptr<SourcePanel> source_panel_;
  std::unique_ptr<WaveTreePanel> wave_tree_panel_;
  std::unique_ptr<WaveSignalsPanel> wave_signals_panel_;
  std::unique_ptr<WavesPanel> waves_panel_;
  struct {
    bool has_waves = false;
    bool has_design = false;
    float f_wave_y = 0.5;
    float f_src_x = 0.3;
    float f_signals_x = 0.15;
    float f_waves_x = 0.28;
    // The wave hierarchy picker panels can be hidden.
    bool show_wave_picker = true;
    // This is calculated from the ratio's above.
    int wave_y;
    int src_x;
    int signals_x;
    int waves_x;
  } layout_;
  // A list of all panels makes it easy to cycle focus.
  std::vector<Panel *> panels_;
  int focused_panel_idx_ = 0;

  // Search state.
  bool searching_ = false;
  TextInput search_box_;
};

} // namespace sv
