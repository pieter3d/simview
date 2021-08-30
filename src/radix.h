#pragma once

#include <string>

namespace sv {

enum class Radix {
  kHex,
  kBinary,
  kUnsignedDecimal,
  kSignedDecimal,
  kFloat,
};

// Format a string of '1' and '0' charachters into the proper representation.
std::string FormatValue(const std::string &bin, Radix radix,
                        bool leading_zeroes);

} // namespace sv
