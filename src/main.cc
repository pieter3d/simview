#include "ui.h"
#include "workspace.h"

int main(int argc, char *argv[]) {
  // Parse the design using the remaining command line arguments
  // TODO: Run in parallel threads with wave parsing?
  // TODO: Skip this if just opening waves.
  std::cout << "Parsing design files..." << std::endl;
  if (!sv::Workspace::Get().ParseDesign(argc,
                                        const_cast<const char **>(argv))) {
    // This function prints plenty of errors if it fails.
    return -1;
  }
  sv::UI ui;
  ui.EventLoop();

  return 0;
}
