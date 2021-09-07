#pragma once

#include <deque>
#include <functional>
#include <ncurses.h>

namespace sv {

// Objects that wish to receive typed text implement this abstract class
// interface.
class TextReceiver {
 public:
  // Inputs are the entered text so far (empty for cancelled) and a flag
  // indicating preview. If this is true, the user is still typing. If false,
  // the user has finished (pressed enter or escape). Returning false
  // indicates that the text was reject and the input text box can be drawn
  // using a negative color. This can be used for searching, to indicate found
  // or not found.
  virtual bool ReceiveText(const std::string &s, bool preview) = 0;
};

// Maintains state for text input. As text is typed (by calling the HandleKey()
// method) the text and cursor state is updated, and the registered Receiver is
// notified.
class TextInput {
 public:
  using Validator = std::function<bool(const std::string &)>;
  enum InputState {
    kTyping,
    kCancelled,
    kDone,
  };
  TextInput() = default;
  TextInput(const std::string &prompt) : prompt_(prompt) {}
  void SetPrompt(const std::string &prompt) { prompt_ = prompt; }
  void SetDims(int row, int col, int width);
  void SetReceiver(TextReceiver *receiver) { receiver_ = receiver; };
  void SetValdiator(Validator v) { validator_ = v; };
  void Reset();
  void Draw(WINDOW *w) const;
  std::pair<int, int> CursorPos() const;
  // Returns current state after the keypress.
  InputState HandleKey(int key);
  const std::string &Text() const { return text_; }
  void SetText(const std::string &s);

 private:
  void FixScroll();

  std::string prompt_;
  int row_ = 0;
  int col_ = 0;
  int width_ = 0;
  TextReceiver *receiver_ = nullptr;
  Validator validator_ = nullptr;
  bool rx_okay_ = false;
  std::string text_;
  std::string saved_text_;
  int cursor_pos_ = 0;
  int scroll_ = 0;
  int history_idx_ = -1;
  std::deque<std::string> history_;
};

} // namespace sv
