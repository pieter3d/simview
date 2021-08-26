#include "wave_signals_panel.h"
#include "color.h"
#include "workspace.h"

namespace sv {

void WaveSignalsPanel::Draw() {
  TreePanel::Draw();
  // Draw the header.
  wmove(w_, 0, 0);
  const std::string toggles = " [sig] [out] [in] [bidir]";
  const bool flags[] = {hide_signals_, hide_outputs_, hide_inputs_,
                        hide_inouts_};
  const int pos[] = {1, 7, 13, 18};
  int idx = 0;
  int max = std::min((int)toggles.size(), getmaxx(w_));
  for (int x = 0; x < max; ++x) {
    if (x == pos[idx]) {
      SetColor(w_, flags[idx] ? kSignalToggleOffPair : kSignalToggleOnPair);
      idx++;
    }
    waddch(w_, toggles[x]);
  }
  wmove(w_, 1, 0);
  SetColor(w_, kSignalFilterPair);
  const std::string filter = "f:";
  max = std::min((int)filter.size(), getmaxx(w_));
  for (int x = 0; x < max; ++x) {
    waddch(w_, filter[x]);
  }
}

WaveSignalsPanel::WaveSignalsPanel() {
  if (Workspace::Get().Waves() == nullptr) return;
  SetScope(&Workspace::Get().Waves()->Root());
  show_expanders_ = false;
  prepend_type_ = true;
  header_lines_ = 2;
}

std::pair<int, int> WaveSignalsPanel::ScrollArea() const {
  int h, w;
  getmaxyx(w_, h, w);
  return {h - header_lines_, w};
}

void WaveSignalsPanel::SetScope(const WaveData::SignalScope *s) {
  scope_ = s;
  data_.Clear();
  signals_.clear();
  for (auto &sig : s->signals) {
    if ((hide_signals_ &&
         sig.direction == WaveData::Signal::Direction::kInternal) ||
        (hide_outputs_ &&
         sig.direction == WaveData::Signal::Direction::kOutput) ||
        (hide_inputs_ &&
         sig.direction == WaveData::Signal::Direction::kInput) ||
        (hide_inouts_ &&
         sig.direction == WaveData::Signal::Direction::kInout)) {
      continue;
    }
    signals_.push_back(SignalTreeItem(sig));
  }
  for (auto &item : signals_) {
    data_.AddRoot(&item);
  }
  SetLineAndScroll(0);
}

void WaveSignalsPanel::UIChar(int ch) {
  switch (ch) {
  case 'w': /* TODO */ break;
  case 'W': /* TODO */ break;
  case 'f': /* TODO */ break;
  case 's':
    hide_signals_ = !hide_signals_;
    SetScope(scope_);
    break;
  case 'o':
    hide_outputs_ = !hide_outputs_;
    SetScope(scope_);
    break;
  case 'i':
    hide_inputs_ = !hide_inputs_;
    SetScope(scope_);
    break;
  case 'b':
    hide_inouts_ = !hide_inouts_;
    SetScope(scope_);
    break;
  default: TreePanel::UIChar(ch);
  }
}

std::string WaveSignalsPanel::Tooltip() const {
  std::string tt = "w:add to waves  ";
  tt += "W:add all to waves  ";
  tt += "f:filter  ";
  tt += "soib:toggle types  ";
  return tt;
}

} // namespace sv
