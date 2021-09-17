#pragma once

#include <fstream>
#include <memory>
#include <string>

namespace sv {

// Reads token from a VCD file.
class VcdTokenizer {
 public:
  explicit VcdTokenizer(const std::string &file_name);
  bool Eof() const;
  std::streampos Position() const;
  void SetPosition(const std::streampos &pos);
  int PosPercentage() const;
  std::string Token();

 private:
  uint64_t file_size_;
  uint64_t chars_read_ = 0;
  std::unique_ptr<std::ifstream> in_;
};

} // namespace sv
