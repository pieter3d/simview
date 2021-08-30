#include "radix.h"
#include "absl/strings/str_format.h"

namespace sv {
std::string FormatValue(const std::string &bin, Radix radix,
                        bool leading_zeroes) {
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
      return absl::StrFormat("%d'd%lu", bin.size(), val);
    } else if (radix == Radix::kSignedDecimal) {
      return absl::StrFormat("%d'd%ld", bin.size(), val);
    } else if (bin.size() == 32) {
      float f;
      memcpy(&f, &val, sizeof(float));
      return absl::StrFormat("32'%g", f);
    } else {
      double d;
      memcpy(&d, &val, sizeof(double));
      return absl::StrFormat("64'%g", d);
    }
  } else if (radix == Radix::kBinary) {
    std::string s = absl::StrFormat("%d'b", bin.size());
    for (int i = bin.size() - 1; i >= 0; --i) {
      s += std::tolower(bin[i]);
      // Add underscore separator every 4 bits.
      if (i % 4 == 0 && i != 0) s += '_';
    }
    return s;
  } else {
    const char hex_digits[] = "0123456789abcdef";
    std::string s = absl::StrFormat("%d'h", bin.size());
    int nibble = 0;
    bool nibble_is_x = false;
    for (int i = bin.size() - 1; i >= 0; --i) {
      const char ch = std::tolower(bin[i]);
      if (ch == 'z' || ch == 'x') nibble_is_x = true;
      nibble |= 1 << (i & 0x3);
      if (i % 4 == 0) {
        s += nibble_is_x ? 'x' : hex_digits[nibble];
        nibble_is_x = false;
        nibble = 0;
        // Add underscore separator every 4 digits (16 bits).
        if (i % 16 == 0 && i != 0) s += '_';
      }
    }
    return s;
  }
}
} // namespace sv
