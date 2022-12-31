#pragma once

#include <uhdm/module_inst.h>
#include <uhdm/uhdm_types.h>

namespace sv {

// Helper utilities to wrangle a UHDM design.

// Returns true if the item is something that might be found in waves and/or be
// considered a net.
bool IsTraceable(const UHDM::any *item);

// Trace the drivers or loads of a given net (or do nothing if the item isn't a
// net).
void GetDriversOrLoads(const UHDM::any *item, bool drivers,
                       std::vector<const UHDM::any *> *list);

// Find the containg module of any item, moving up through generate blocks if
// necessary.
const UHDM::module_inst *GetContainingModule(const UHDM::any *item);

// Find the containing scope (module or genblock) of any item.
const UHDM::any *GetScopeForUI(const UHDM::any *item);

} // namespace sv
