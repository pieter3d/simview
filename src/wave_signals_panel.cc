#include "wave_signals_panel.h"
#include "color.h"
#include "workspace.h"
#include <regex>

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
  if (editing_filter_) {
    filter_input_.Draw(w_);
  } else {
    SetColor(w_, kSignalFilterPair);
    const std::string filter = "filter:" + filter_text_;
    max = std::min((int)filter.size(), getmaxx(w_));
    for (int x = 0; x < max; ++x) {
      waddch(w_, filter[x]);
    }
  }
}

WaveSignalsPanel::WaveSignalsPanel() : filter_input_("filter:") {
  if (Workspace::Get().Waves() == nullptr) return;
  SetScope(&Workspace::Get().Waves()->Root());
  show_expanders_ = false;
  prepend_type_ = true;
  header_lines_ = 2;
  filter_input_.SetDims(1, 0, getmaxx(w_));
}

std::pair<int, int> WaveSignalsPanel::ScrollArea() const {
  int h, w;
  getmaxyx(w_, h, w);
  return {h - header_lines_, w};
}

void WaveSignalsPanel::Resized() { filter_input_.SetDims(1, 0, getmaxx(w_)); }

std::optional<std::pair<int, int>> WaveSignalsPanel::CursorLocation() const {
  if (!editing_filter_) return std::nullopt;
  return filter_input_.CursorPos();
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
    if (!filter_text_.empty()) {
      // If the filter starts with a leading /, it's a regular expression.
      if (filter_text_[0] == '/') {
        std::regex r(filter_text_.substr(1));
        if (!std::regex_search(sig.name, r)) continue;
      } else {
        // Skip anything that doesn't match.
        if (sig.name.find(filter_text_) == std::string::npos) continue;
      }
    }
    signals_.push_back(SignalTreeItem(sig));
  }
  for (auto &item : signals_) {
    data_.AddRoot(&item);
  }
  SetLineAndScroll(0);
}

void WaveSignalsPanel::UIChar(int ch) {
  if (editing_filter_) {
    const auto state = filter_input_.HandleKey(ch);
    if (state != TextInput::kTyping) {
      editing_filter_ = false;
      if (state == TextInput::kDone) {
        filter_text_ = filter_input_.Text();
        // Re-process the current scope.
        SetScope(scope_);
      }
    }
  } else {
    switch (ch) {
    case 'w': /* TODO */ break;
    case 'W': /* TODO */ break;
    case 'f': editing_filter_ = true; break;
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
}

std::string WaveSignalsPanel::Tooltip() const {
  std::string tt = "w:add to waves  ";
  tt += "W:add all to waves  ";
  tt += "f:filter  ";
  tt += "soib:toggle types  ";
  return tt;
}

} // namespace sv
