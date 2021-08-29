#pragma once

#include <uhdm/headers/module.h>
#include <uhdm/headers/uhdm_types.h>

namespace sv {

// ------------- UHDM Utilities --------------
bool IsTraceable(const UHDM::any *item);
void GetDriversOrLoads(const UHDM::any *item, bool drivers,
                       std::vector<const UHDM::any *> &list);
const UHDM::module *GetContainingModule(const UHDM::any *item);
const UHDM::any *GetScopeForUI(const UHDM::any *item);
bool EquivalentNet(const UHDM::any *a, const UHDM::any *b);

} // namespace sv
