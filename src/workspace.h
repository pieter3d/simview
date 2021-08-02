#ifndef _SRC_WORKSPACE_H_
#define _SRC_WORKSPACE_H_

#include <uhdm/headers/design.h>

namespace sv {

struct Workspace {
  UHDM::design *design;
  std::vector<std::string> include_paths;
};

} // namespace sv
#endif // _SRC_WORKSPACE_H_
