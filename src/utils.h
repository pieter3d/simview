#ifndef _SRC_UTILS_H_
#define _SRC_UTILS_H_

#include <string>
#include <uhdm/headers/module.h>
#include <uhdm/headers/uhdm_types.h>
#include <vector>

namespace sv {

// Removes the work@ prefix from a string.
std::string StripWorklib(const std::string &s);

bool IsTraceable(const UHDM::any *item);
void GetDriversOrLoads(const UHDM::any *item, bool drivers,
                       std::vector<const UHDM::any *> &list);
const UHDM::module *GetContainingModule(const UHDM::any *item);
const UHDM::any *GetScopeForUI(const UHDM::any *item);
bool EquivalentNet(const UHDM::any *a, const UHDM::any *b);

} // namespace sv
#endif // _SRC_UTILS_H_
