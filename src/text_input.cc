#include "text_input.h"
#include "color.h"

namespace sv {

namespace {
constexpr int kMaxSeachHistorySize = 500;
}

void TextInput::SetDims(int row, int col, int width) {
  row_ = row;
  col_ = col;
  width_ = width;
  FixScroll();
}

void TextInput::Reset() {
  cursor_pos_ = 0;
  scroll_ = 0;
  history_idx_ = -1;
  text_ = "";
  rx_okay_ = false;
}

void TextInput::SetText(const std::string &s) {
  text_ = s;
  cursor_pos_ = text_.size();
  FixScroll();
}

void TextInput::Draw(WINDOW *w) const {
  SetColor(w, (text_.empty() || rx_okay_ ||
               (receiver_ == nullptr && validator_ == nullptr))
                  ? kTextInputPair
                  : kTextInputRejectPair);
  wmove(w, row_, col_);
  for (int x = 0; x < width_; ++x) {
    if (x < prompt_.size()) {
      waddch(w, prompt_[x]);
    } else {
      const int pos = x - prompt_.size() + scroll_;
      waddch(w, pos < text_.size() ? text_[pos] : ' ');
    }
  }
}

std::pair<int, int> TextInput::CursorPos() const {
  return {row_, col_ + prompt_.size() + cursor_pos_ - scroll_};
}

void TextInput::FixScroll() {
  // Scroll the search box so that the cursor is inside.
  if (cursor_pos_ < scroll_) {
    scroll_ = cursor_pos_ - std::min(width_ / 3, 8);
  } else if (cursor_pos_ - scroll_ >= width_ - prompt_.size()) {
    scroll_ = cursor_pos_ - width_ + 1 + prompt_.size();
  }
}

TextInput::InputState TextInput::HandleKey(int key) {
  bool text_changed = false;
  InputState state = kTyping;
  // TextInputing is modal, so do nothing else until that is handled.
  switch (key) {
  case 0x104: // left
    if (cursor_pos_ != 0) {
      cursor_pos_--;
      FixScroll();
    }
    break;
  case 0x105: // right
    if (cursor_pos_ < text_.size()) {
      cursor_pos_++;
      FixScroll();
    }
    break;
  case 0x106: // home
    cursor_pos_ = 0;
    FixScroll();
    break;
  case 0x168: // end
    cursor_pos_ = text_.size();
    FixScroll();
    break;
  case 0x103: // up
    if (history_idx_ < static_cast<int>(history_.size() - 1)) {
      if (history_idx_ == -1) saved_text_ = text_;
      history_idx_++;
      text_ = history_[history_idx_];
      cursor_pos_ = text_.size();
    }
    break;
  case 0x102: // down
    if (history_idx_ >= 0) {
      history_idx_--;
      if (history_idx_ < 0) {
        text_ = saved_text_;
      } else {
        text_ = history_[history_idx_];
      }
      cursor_pos_ = text_.size();
    }
    break;
  case 0x107: // backspace
    if (cursor_pos_ > 0) {
      cursor_pos_--;
      FixScroll();
      text_.erase(cursor_pos_, 1);
      text_changed = true;
    } else {
      // Backspace empty = cancel search.
      state = kCancelled;
    }
    break;
  case 0x14a: // delete
    if (cursor_pos_ < text_.size()) {
      text_.erase(cursor_pos_, 1);
      text_changed = true;
    }
    break;
  case 0x3:   // Ctrl-C
  case 0x1b:  // Escape
  case 0xd: { // Enter
    const bool cancelled = key != 0xd;
    state = cancelled ? kCancelled : kDone;
    if (!cancelled) {
      history_.push_front(text_);
      if (history_.size() > kMaxSeachHistorySize) {
        history_.pop_back();
      }
    }
  } break;
  default:
    if (key <= 0xff) {
      text_.insert(cursor_pos_, 1, static_cast<char>(key));
      cursor_pos_++;
      FixScroll();
      text_changed = true;
    }
    break;
  }
  if (receiver_ != nullptr) {
    if (state == kTyping && text_changed) {
      rx_okay_ = receiver_->ReceiveText(text_, /*preview*/ true);
    } else if (state != kTyping) {
      receiver_->ReceiveText(text_, /*preview*/ false);
      Reset();
    }
  } else if (validator_ != nullptr && state == kTyping && text_changed) {
    rx_okay_ = validator_(text_);
  }
  return state;
}

} // namespace sv
