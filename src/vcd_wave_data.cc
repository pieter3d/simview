#include "vcd_wave_data.h"
#include <filesystem>
#include <ios>
#include <stdexcept>

namespace sv {
namespace {
const std::runtime_error kParseError("VCD file parsing error, file malformed.");
}

// Default: print.
bool VcdWaveData::print_progress_ = true;

VcdWaveData::VcdWaveData(const std::string &file_name)
    : tokenizer_(VcdTokenizer(file_name)) {
  // Start reading declaration commands until all declarations are done.
  while (true) {
    auto tok = tokenizer_.Token();
    if (tok == "$comment" || tok == "$date" || tok == "$version") {
      ParseToEofCommand();
    } else if (tok == "$timescale") {
      ParseTimescale();
    } else if (tok == "$var") {
      ParseVariable();
    } else if (tok == "$scope") {
      ParseScope();
    } else if (tok == "$upscope") {
      ParseUpScope();
    } else if (tok == "$enddefinitions") {
      ParseToEofCommand();
      break;
    } else {
      throw std::runtime_error("VCD parsing error in declarations");
    }
  }
  // Clear the stack. In practice this should already be empty since properly
  // formatted files should have upscope commands to do this. However, it isn't
  // really a problem if the last ones are missing.
  while (!scope_stack_.empty())
    scope_stack_.pop();
  // Traverse the completed scope/signal list, and assign parents.
  BuildParents();
  ParseSimCommands();
}

void VcdWaveData::LoadSignalSamples(const std::vector<const Signal *> &signals,
                                    uint64_t start_time,
                                    uint64_t end_time) const {
  // Do nothing, all waves are parsed in upon file load.
  // TODO: Lazy / on-demand loading might be useful.
}

void VcdWaveData::ParseToEofCommand() {
  while (!tokenizer_.Eof()) {
    auto tok = tokenizer_.Token();
    if (tok == "$end") return;
  }
}

void VcdWaveData::ParseScope() {
  auto tok = tokenizer_.Token();
  std::string name = "[unnamed]";
  if (tok != "$end") {
    // Ignore current value of tok, the scope type.
    name = tokenizer_.Token();
    std::string end = tokenizer_.Token();
    if (end != "$end") {
      throw kParseError;
    }
  }
  if (scope_stack_.empty()) {
    // New root level scope.
    roots_.push_back({});
    roots_.back().name = name;
    scope_stack_.push(&roots_.back());
  } else {
    scope_stack_.top()->children.push_back({});
    scope_stack_.push(&scope_stack_.top()->children.back());
    scope_stack_.top()->name = name;
  }
}

void VcdWaveData::ParseUpScope() {
  auto tok = tokenizer_.Token();
  if (scope_stack_.empty() || tok != "$end") {
    throw kParseError;
  }
  scope_stack_.pop();
}

void VcdWaveData::ParseVariable() {
  tokenizer_.Token(); // Discard the type;
  int var_size = std::stoi(tokenizer_.Token());
  auto code = tokenizer_.Token();
  auto name = tokenizer_.Token();
  // After the reference identifier, optional bit select index may be present.
  // Append these. At most 5 more tokens in the case of name [ msb : lsb ],
  // with all spaces between them.
  std::string tok;
  int tokens_read = 0;
  while (true) {
    tok = tokenizer_.Token();
    tokens_read++;
    if (tok == "$end" || tokens_read > 5) break;
    name += tok;
  }
  if (tok != "$end") {
    throw kParseError;
  }
  scope_stack_.top()->signals.push_back({});
  auto &s = scope_stack_.top()->signals.back();
  s.width = var_size;
  s.name = name;
  // VCD files don't have this info.
  s.type = Signal::kNet;
  s.direction = Signal::kInternal;
  // See if there is an LSB index to parse.
  auto range_pos = name.find_last_of('[');
  auto colon_pos = name.find_last_of(':');
  if (range_pos != std::string::npos && colon_pos != std::string::npos &&
      range_pos < colon_pos) {
    s.lsb = std::stoi(name.substr(colon_pos + 1));
    s.has_suffix = true;
  }
  if (signal_id_by_code_.find(code) == signal_id_by_code_.end()) {
    signal_id_by_code_[code] = current_id_;
    s.id = current_id_;
    current_id_++;
  } else {
    s.id = signal_id_by_code_[code];
  }
}

void VcdWaveData::ParseTimescale() {
  auto tok = tokenizer_.Token();
  size_t chars_read;
  int val = std::stoi(tok, &chars_read);
  if (val != 1 && val != 10 && val != 100) {
    throw kParseError;
  }
  // Time unit could be another token.
  if (chars_read < tok.size()) {
    tok = tok.substr(chars_read);
  } else {
    tok = tokenizer_.Token();
  }
  if (tok == "s") {
    time_units_ = 0;
  } else if (tok == "ms") {
    time_units_ = -3;
  } else if (tok == "us") {
    time_units_ = -6;
  } else if (tok == "ns") {
    time_units_ = -9;
  } else if (tok == "ps") {
    time_units_ = -12;
  } else if (tok == "fs") {
    time_units_ = -15;
  } else {
    throw kParseError;
  }
  if (val == 10) {
    time_units_ += 1;
  } else if (val == 100) {
    time_units_ += 2;
  }
  tok = tokenizer_.Token();
  if (tok != "$end") {
    throw kParseError;
  }
}

void VcdWaveData::ParseSimCommands() {
  bool in_dump = false;
  uint64_t time = 0;
  bool first_time = true;
  int prev_percentage = -1;
  while (!tokenizer_.Eof()) {
    auto tok = tokenizer_.Token();
    if (tok.empty()) continue;
    if (!in_dump && tok.find("$dump") == 0) {
      in_dump = true;
    } else if (in_dump && tok == "$end") {
      in_dump = false;
    } else if (tok == "$comment") {
      ParseToEofCommand();
    } else if (tok[0] == '#') {
      time = std::stol(tok.substr(1));
      if (first_time) {
        first_time = false;
        time_range_.first = time;
      }
    } else if (tok.find_first_of("bBrR") == 0) {
      Sample s;
      s.time = time;
      s.value = tok.substr(1);
      tok = tokenizer_.Token();
      if (signal_id_by_code_.find(tok) == signal_id_by_code_.end()) {
        throw kParseError;
      }
      waves_[signal_id_by_code_[tok]].push_back(s);
    } else if (tok.find_first_of("01xXzZ") == 0) {
      Sample s;
      s.time = time;
      s.value = tok[0];
      const auto id_code = tok.substr(1);
      const auto id = signal_id_by_code_.find(id_code);
      if (id == signal_id_by_code_.end()) {
        throw kParseError;
      }
      waves_[id->second].push_back(s);
    } else {
      throw kParseError;
    }
    if (print_progress_) {
      int percentage = tokenizer_.PosPercentage();
      if (percentage != prev_percentage) {
        printf("%d%%\r", percentage);
        fflush(stdout);
        prev_percentage = percentage;
      }
    }
  }
  // Avoid start > end.
  time_range_.second = std::max(time_range_.first + 1, time);
  if (print_progress_) {
    printf("\n");
  }
}

} // namespace sv
