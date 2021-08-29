#include "utils.h"
#include "absl/strings/str_format.h"

namespace sv {

std::string StripWorklib(const std::string &s) {
  const int lib_delimieter_pos = s.find('@');
  if (lib_delimieter_pos == std::string::npos) return s;
  return s.substr(lib_delimieter_pos + 1);
}

int NumDecimalDigits(int n) {
  int ret = 0;
  do {
    ret++;
    n /= 10;
  } while (n != 0);
  return ret;
}

std::string AddDigitSeparators(uint64_t val) {
  std::string val_digits = absl::StrFormat("%ld", val);
  std::string val_with_separators;
  for (int i = 0; i < val_digits.size(); ++i) {
    val_with_separators += val_digits[i];
    if (i < (val_digits.size() - 1) && (val_digits.size() - 1 - i) % 3 == 0) {
      val_with_separators += ',';
    }
  }
  return val_with_separators;
}

std::optional<uint64_t> ParseTime(const std::string &s, int smallest_unit) {
  if (s.empty()) return std::nullopt;
  std::string t;
  // First strip any underscores or apostrophe's.
  for (const char c : s) {
    if (c != '_' && c != '\'' && c != ',') t += std::tolower(c);
  }
  // See if some kind of unit was typed.
  int suffix_pos = t.size();
  while (suffix_pos > 0 &&
         (t[suffix_pos - 1] < '0' || t[suffix_pos - 1] > '9')) {
    suffix_pos--;
  }
  int units = smallest_unit;
  if (suffix_pos > 0 && suffix_pos < t.size()) {
    const std::string suffix = t.substr(suffix_pos);
    if (suffix == "as") {
      units = -18;
    } else if (suffix == "fs") {
      units = -15;
    } else if (suffix == "ps") {
      units = -12;
    } else if (suffix == "ns") {
      units = -9;
    } else if (suffix == "us") {
      units = -6;
    } else if (suffix == "ms") {
      units = -3;
    } else if (suffix == "s") {
      units = 0;
    } else if (suffix == "ks") {
      units = 3;
    } else {
      return std::nullopt;
    }
  }
  try {
    const uint64_t val = std::stol(t.substr(0, suffix_pos));
    return pow(10, units - smallest_unit) * val;
  } catch (std::exception &e) {
  }
  return std::nullopt;
}

} // namespace sv
