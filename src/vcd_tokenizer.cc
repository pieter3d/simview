#include "vcd_tokenizer.h"
#include <fstream>

namespace sv {
namespace {
inline bool is_whitespace(char ch) {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}
} // namespace

VcdTokenizer::VcdTokenizer(const std::string &file_name) {
  in_ = std::make_unique<std::ifstream>(file_name);
  if (!in_->is_open()) {
    throw std::runtime_error("Unable to read VCD file.");
  }
}

bool VcdTokenizer::Eof() const { return in_->eof(); }

std::streampos VcdTokenizer::Position() const { return in_->tellg(); }

void VcdTokenizer::SetPosition(const std::streampos &pos) {
  // Failbit is set by get() when reading end of file, so have to clear state
  // in order for seekg to work.
  if (in_->eof()) in_->clear();
  in_->seekg(pos);
}

std::string VcdTokenizer::Token() {
  std::string tok;
  char ch;
  while (true) {
    ch = in_->get();
    if (in_->eof()) return tok;
    // Skip past leading whitespace
    if (tok.empty() && is_whitespace(ch)) continue;
    if (is_whitespace(ch)) return tok;
    tok += ch;
  }
}

} // namespace sv
