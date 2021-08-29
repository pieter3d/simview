#pragma once

#include <functional>
#include <string>
#include <uhdm/headers/module.h>
#include <uhdm/headers/uhdm_types.h>
#include <vector>

namespace sv {

// Removes the work@ prefix from a string.
std::string StripWorklib(const std::string &s);
int NumDecimalDigits(int n);
std::string AddDigitSeparators(uint64_t val);
// Attempt to parse time. If successful, returns the time and the log10 units.
// If the string doesn't contain any specific units, the default units are used.
std::optional<std::pair<uint64_t, int>> ParseTime(const std::string &s,
                                                  int default_unit);

// ------------- UHDM Utilities --------------
bool IsTraceable(const UHDM::any *item);
void GetDriversOrLoads(const UHDM::any *item, bool drivers,
                       std::vector<const UHDM::any *> &list);
const UHDM::module *GetContainingModule(const UHDM::any *item);
const UHDM::any *GetScopeForUI(const UHDM::any *item);
bool EquivalentNet(const UHDM::any *a, const UHDM::any *b);

} // namespace sv
