#include "radix.h"
#include "absl/strings/str_format.h"

namespace sv {
std::string FormatValue(const std::string &bin, Radix radix,
                        bool leading_zeroes, bool drop_size) {
  if (radix == Radix::kUnsignedDecimal || radix == Radix::kSignedDecimal ||
      radix == Radix::kFloat) {
    // These formats can use a real machine type and therefore take advantage of
    // printf style formatting.
    uint64_t val = 0;
    for (int i = 0; i < bin.size(); ++i) {
      const char ch = std::tolower(bin[bin.size() - 1 - i]);
      if (ch == 'z' || ch == 'x') return "x";
      if (bin[bin.size() - 1 - i] == '1') val |= 1ull << i;
    }
    if (radix == Radix::kUnsignedDecimal) {
      return (drop_size ? "" : absl::StrFormat("%d'd", bin.size())) +
             absl::StrFormat("%lu", val);
    } else if (radix == Radix::kSignedDecimal) {
      return (drop_size ? "" : absl::StrFormat("%d'sd", bin.size())) +
             absl::StrFormat("%ld", val);
    } else if (bin.size() == 32) {
      float f;
      memcpy(&f, &val, sizeof(float));
      return absl::StrFormat("%g", f);
    } else {
      double d;
      memcpy(&d, &val, sizeof(double));
      return absl::StrFormat("%g", d);
    }
  } else if (radix == Radix::kBinary) {
    std::string s;
    for (int i = 0; i < bin.size(); ++i) {
      s += std::tolower(bin[i]);
      // Add underscore separator every 4 bits.
      const int bitpos = bin.size() - 1 - i;
      if (bitpos % 4 == 0 && bitpos != 0) s += '_';
    }
    return drop_size ? s : (absl::StrFormat("%d'b", bin.size()) + s);
  } else {
    const char hex_digits[] = "0123456789abcdef";
    std::string hex;
    int nibble = 0;
    bool nibble_is_x = false;
    for (int i = 0; i < bin.size(); ++i) {
      const char ch = std::tolower(bin[i]);
      const int bitpos = bin.size() - 1 - i;
      if (ch == 'z' || ch == 'x') nibble_is_x = true;
      if (ch == '1') nibble |= 1 << (bitpos & 0x3);
      if (bitpos % 4 == 0) {
        hex += nibble_is_x ? 'x' : hex_digits[nibble];
        nibble_is_x = false;
        nibble = 0;
        // Add underscore separator every 4 digits (16 bits).
        if (bitpos % 16 == 0 && bitpos != 0) hex += '_';
      }
    }
    // Strip any leading zeroes if needed.
    if (!leading_zeroes) {
      const int pos = hex.find_first_not_of("0_");
      if (pos == std::string::npos) {
        hex = "0";
      } else {
        hex = hex.substr(pos);
      }
    }
    return drop_size ? hex : (absl::StrFormat("%d'h", bin.size()) + hex);
  }
}
} // namespace sv
