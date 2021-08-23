#include "ui.h"
#include "workspace.h"

int main(int argc, const char *argv[]) {
  // Parse the design using the remaining command line arguments

  // TODO: Skip this if just opening waves.
  if (!sv::Workspace::Get().ParseDesign(argc, argv)) {
    return -1;
  }
  sv::UI ui;
  ui.EventLoop();

  return 0;
}
