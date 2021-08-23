#ifndef _SRC_WORKSPACE_H_
#define _SRC_WORKSPACE_H_

#include <uhdm/headers/design.h>
#include <surelog/surelog.h>
#include <unordered_map>
#include <vector>

namespace sv {

class Workspace {
 public:
  static Workspace &Get() {
    static Workspace w;
    return w;
  }

  // Parse the design using command line arguments for Surelog.
  // Return true on success.
  bool ParseDesign(int argc, const char *argv[]);
  const UHDM::design *Design() const { return design_; }
  // Find the definition of the module that contains the given item.
  const UHDM::module *GetDefinition(const UHDM::module *m);

  void AddIncludeDir(std::string d) { include_paths_.push_back(d); }
  const auto &IncludeDirs() const { return include_paths_; }

 private:
  // Singleton
  Workspace() {}
  ~Workspace();
  // Track all definitions of any given module instance.
  // This serves as a cache to avoid iterating over the
  // design's list of all module definitions.
  std::unordered_map<std::string, const UHDM::module *> module_defs_;
  UHDM::design *design_ = nullptr;
  SURELOG::SymbolTable symbol_table_;
  SURELOG::scompiler *compiler_ = nullptr;
  std::vector<std::string> include_paths_;
};

} // namespace sv
#endif // _SRC_WORKSPACE_H_
