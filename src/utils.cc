#include "utils.h"
#include "absl/strings/str_format.h"
#include <filesystem>
#include <optional>
#include <wordexp.h>

namespace sv {

int NumDecimalDigits(int n) { return 1 + std::floor(std::log10(n)); }

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
  while (suffix_pos > 0 && (t[suffix_pos - 1] < '0' || t[suffix_pos - 1] > '9')) {
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

std::optional<std::string> ActualFileName(const std::string &file_name, bool allow_noexist) {
  wordexp_t results;
  // Don't run any commands, that's weird in this context.
  int ret = wordexp(file_name.c_str(), &results, WRDE_NOCMD);
  std::string expanded_name;
  if (results.we_wordc > 0) expanded_name = results.we_wordv[0];
  wordfree(&results);
  if (ret != 0) return std::nullopt;
  if (expanded_name.empty()) return std::nullopt;
  if (!allow_noexist && !std::filesystem::exists(expanded_name)) return std::nullopt;
  return expanded_name;
}

std::optional<std::string> ActualFileName(const std::string &file_name) {
  return ActualFileName(file_name, /*allow_noexist*/ false);
}

} // namespace sv
