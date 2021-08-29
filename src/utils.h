#pragma once

#include <optional>
#include <string>
#include <vector>

namespace sv {

// Removes the work@ prefix from a string.
std::string StripWorklib(const std::string &s);
int NumDecimalDigits(int n);
std::string AddDigitSeparators(uint64_t val);
// Attempt to parse time. If successful, returns the time and the log10 units.
// If the string doesn't contain any specific units, the default units are used.
std::optional<uint64_t> ParseTime(const std::string &s, int smallest_unit);

} // namespace sv
