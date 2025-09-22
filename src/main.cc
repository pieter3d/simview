#include "ui.h"
#include "workspace.h"

int main(int argc, char *argv[]) {
  if (!sv::Workspace::Get().ParseDesign(argc, argv)) {
    // This function prints plenty of errors if it fails.
    return -1;
  }

  // Start the UI and the event loop.
  sv::UI ui;
  ui.EventLoop();
  if (!ui.FinalMessage().empty()) {
    std::cout << ui.FinalMessage() << "\n";
  }

  return 0;
}
