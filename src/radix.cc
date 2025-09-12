#include "radix.h"
#include "absl/strings/str_format.h"
#include <optional>

namespace sv {

namespace {

int64_t SignExtend(uint64_t val, int bits) {
  if (val & (1ull << (bits - 1))) return val | (static_cast<uint64_t>(-1) << bits);
  return val;
}

template <typename T>
std::optional<T> BinStringCast(std::string_view bin) {
  std::optional<uint64_t> val_or = BinStringToUnsigned(bin);
  if (!val_or) return std::nullopt;
  const uint64_t val = *val_or;
  T t;
  memcpy(&t, &val, sizeof(T));
  return t;
}

} // namespace

std::optional<uint64_t> BinStringToUnsigned(std::string_view bin) {
  if (bin.size() > 64) return std::nullopt;
  uint64_t val = 0;
  for (char ch : bin) {
    if (ch != '0' && ch != '1') return std::nullopt;
    val <<= 1;
    val |= ch == '1';
  }
  return val;
}

std::optional<float> BinStringToFp32(std::string_view bin) { return BinStringCast<float>(bin); }

std::optional<double> BinStringToFp64(std::string_view bin) { return BinStringCast<double>(bin); }

std::optional<int64_t> BinStringToSigned(std::string_view bin) {
  std::optional<uint64_t> val_or = BinStringToUnsigned(bin);
  if (!val_or) return std::nullopt;
  return SignExtend(*val_or, bin.size());
}

std::string FormatValue(const std::string &bin, Radix radix, bool leading_zeroes, bool drop_size) {
  if (radix == Radix::kUnsignedDecimal || radix == Radix::kSignedDecimal ||
      radix == Radix::kFloat) {
    // These formats can use a real machine type and therefore take advantage of
    // printf style formatting.
    std::optional<uint64_t> maybe_val = BinStringToUnsigned(bin);
    if (!maybe_val) return "x";
    uint64_t val = *maybe_val;
    if (radix == Radix::kUnsignedDecimal) {
      return (drop_size ? "" : absl::StrFormat("%d'd", bin.size())) + absl::StrFormat("%lu", val);
    } else if (radix == Radix::kSignedDecimal) {
      int64_t sval = SignExtend(val, bin.size());
      return (drop_size ? "" : absl::StrFormat("%d'sd", bin.size())) + absl::StrFormat("%ld", sval);
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
    bool drop_zeroes = !leading_zeroes;
    for (int i = 0; i < bin.size(); ++i) {
      if (drop_zeroes && bin[i] == '0' && i != bin.size() - 1) continue;
      drop_zeroes &= bin[i] == '0';
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

Radix CharToRadix(char c) {
  switch (c) {
  case 'b': return Radix::kBinary;
  case 'u': return Radix::kUnsignedDecimal;
  case 's': return Radix::kSignedDecimal;
  case 'f': return Radix::kFloat;
  default: return Radix::kHex;
  }
}

char RadixToChar(Radix r) {
  switch (r) {
  case Radix::kBinary: return 'b';
  case Radix::kUnsignedDecimal: return 'u';
  case Radix::kSignedDecimal: return 's';
  case Radix::kFloat: return 'f';
  default: return 'h';
  }
}

} // namespace sv
