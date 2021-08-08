#include "workspace.h"
#include <uhdm/headers/module.h>

namespace sv {

const UHDM::module *Workspace::GetDefinition(const UHDM::module *m) {
  // Top modules don't have a separate definition.
  if (m->VpiTopModule()) return m;
  const std::string &def_name = m->VpiDefName();
  if (module_defs_.find(def_name) == module_defs_.end()) {
    // Find the module definition in the UHDB.
    for (auto &candidate_module : *design_->AllModules()) {
      if (def_name == candidate_module->VpiDefName()) {
        module_defs_[def_name] = candidate_module;
        return candidate_module;
      }
    }
  }
  return module_defs_[def_name];
}

} // namespace sv
