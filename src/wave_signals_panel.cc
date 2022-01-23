#include "wave_signals_panel.h"
#include "color.h"
#include "workspace.h"
#include <algorithm>
#include <regex>

namespace sv {

void WaveSignalsPanel::Draw() {
  TreePanel::Draw();
  // Draw the header.
  wmove(w_, 0, 0);
  const std::string toggles = " [net] [in] [out] [inout]";
  const bool flags[] = {hide_signals_, hide_inputs_, hide_outputs_,
                        hide_inouts_};
  const int pos[] = {1, 7, 12, 18};
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
  if (Workspace::Get().Waves()->Roots().empty()) return;
  SetScope(&Workspace::Get().Waves()->Roots()[0]);
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
  items_.clear();
  for (auto &sig : s->signals) {
    if ((hide_signals_ && sig.direction == WaveData::Signal::kInternal) ||
        (hide_outputs_ && sig.direction == WaveData::Signal::kOutput) ||
        (hide_inputs_ && sig.direction == WaveData::Signal::kInput) ||
        (hide_inouts_ && sig.direction == WaveData::Signal::kInout)) {
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
    items_.push_back(SignalTreeItem(&sig));
  }
  if (sort_) {
    std::sort(items_.begin(), items_.end(),
              [](const SignalTreeItem &a, const SignalTreeItem &b) {
                return a.Name() < b.Name();
              });
  }
  for (auto &item : items_) {
    data_.AddRoot(&item);
  }
  SetLineAndScroll(0);
}

std::optional<std::vector<const WaveData::Signal *>>
WaveSignalsPanel::SignalsForWaves() {
  if (!add_signals_) return std::nullopt;
  std::vector<const WaveData::Signal *> ret;
  for (int i = add_signal_range_.first; i <= add_signal_range_.second; ++i) {
    ret.push_back(items_[i].Signal());
  }
  add_signals_ = false;
  return ret;
}

void WaveSignalsPanel::UIChar(int ch) {
  bool cancel_multi_line = true;
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
      if (line_idx_ != items_.size() - 1) {
        if (multi_line_idx_ < 0) multi_line_idx_ = line_idx_;
        Panel::UIChar('j');
        cancel_multi_line = false;
      }
      break;
    case 'w':
    case 'W': {
      const bool add_all = ch == 'W';
      const int mult = multi_line_idx_ < 0 ? line_idx_ : multi_line_idx_;
      add_signal_range_.first = add_all ? 0 : std::min(line_idx_, mult);
      add_signal_range_.second =
          add_all ? items_.size() - 1 : std::max(line_idx_, mult);
      add_signals_ = true;
    } break;
    case 'f': editing_filter_ = true; break;
    case '1':
      hide_signals_ = !hide_signals_;
      SetScope(scope_);
      break;
    case '2':
      hide_inputs_ = !hide_inputs_;
      SetScope(scope_);
      break;
    case '3':
      hide_outputs_ = !hide_outputs_;
      SetScope(scope_);
      break;
    case '4':
      hide_inouts_ = !hide_inouts_;
      SetScope(scope_);
      break;
    case 's':
      sort_ = !sort_;
      SetScope(scope_);
      break;
    default: TreePanel::UIChar(ch);
    }
  }
  if (cancel_multi_line) multi_line_idx_ = -1;
}

std::vector<Tooltip> WaveSignalsPanel::Tooltips() const {
  std::vector<Tooltip> tt{{"w", "add to waves"},
                          {"W", "add all to waves"},
                          {"f", "filter"},
                          {"1234", "toggle types"},
                          {"s", "toggle sort"}};
  return tt;
}

} // namespace sv
