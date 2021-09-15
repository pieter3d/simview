#pragma once

#include <string>

namespace sv {

enum class Radix {
  kHex = 0,
  kBinary,
  kUnsignedDecimal,
  kSignedDecimal,
  kFloat,
};

// Format a string of '1' and '0' charachters into the proper representation.
std::string FormatValue(const std::string &bin, Radix radix,
                        bool leading_zeroes, bool drop_size = false);

// Simple mnemonic used for signal list file saving/loading.
Radix CharToRadix(char c);
char RadixToChar(Radix r);

} // namespace sv
