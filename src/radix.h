#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace sv {

enum class Radix {
  kHex = 0,
  kBinary,
  kUnsignedDecimal,
  kSignedDecimal,
  kFloat,
};

std::optional<float> BinStringToFp32(std::string_view bin);
std::optional<double> BinStringToFp64(std::string_view bin);
std::optional<int64_t> BinStringToSigned(std::string_view bin);
std::optional<uint64_t> BinStringToUnsigned(std::string_view bin);

// Format a string of '1' and '0' charachters into the proper representation.
std::string FormatValue(const std::string &bin, Radix radix,
                        bool leading_zeroes, bool drop_size = false);

// Simple mnemonic used for signal list file saving/loading.
Radix CharToRadix(char c);
char RadixToChar(Radix r);

} // namespace sv
