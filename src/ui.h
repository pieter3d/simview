#pragma once

#include "design_tree_panel.h"
#include "source_panel.h"
#include "text_input.h"
#include "wave_signals_panel.h"
#include "wavedata_tree_panel.h"
#include "waves_panel.h"
#include <memory>

namespace sv {

// This is the main UI. It owns all UI sub elements, all of which decend from sv::Panel. It responds
// to terminal resizing and attempts to keep the layout properly balanced.
class UI {
 public:
  // Constructor enters NCurses mode.
  UI();

  // This is the main event loop. This function doesn't return until the user has pressed Ctrl-Q to
  // exit the UI. A closing message is returned, in case of some kind of error.
  void EventLoop();
  const std::string &FinalMessage() const { return final_message_; }

 private:
  void CalcLayout(bool update_frac = false);
  void LayoutPanels();
  void CycleFocus(bool fwd);
  void UpdateTooltips();
  void Draw() const;
  void DrawHelp(int panel_idx) const;
  // False if some fatal error requires abort.
  bool Reload();

  std::unique_ptr<DesignTreePanel> design_tree_panel_;
  std::unique_ptr<SourcePanel> source_panel_;
  std::unique_ptr<WaveDataTreePanel> wave_tree_panel_;
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
  std::string error_message_;
  // State for tooltips
  int tooltips_to_show_;
  std::vector<Tooltip> tooltips_;
  bool draw_tooltips_ = false;
  int tooltip_start_idx_ = 0;

  // Search state.
  bool searching_ = false;
  TextInput search_box_;

  // Terminating message for the user after the UI exits.
  std::string final_message_;
};

} // namespace sv
