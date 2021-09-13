#pragma once

#include <fstream>
#include <memory>
#include <string>

namespace sv {

class VcdTokenizer {
 public:
  explicit VcdTokenizer(const std::string &file_name);
  bool Eof() const;
  std::streampos Position() const;
  void SetPosition(const std::streampos &pos);
  std::string Token();

 private:
  std::unique_ptr<std::ifstream> in_;
};

} // namespace sv
