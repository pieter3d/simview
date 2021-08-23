#include "ui.h"
#include "workspace.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout
        << "simview: view and browse RTL design code and simulation waves.\n"
           "Specify an FST or VCD file with -waves, and/or design parsing "
           "flags to "
           "load verilog RTL.\n";
    return -1;
  }
  std::vector<char *> pruned_args;
  // First look for a -waves argument.
  std::string wave_file;
  for (int i = 0; i < argc; ++i) {
    if (strcmp(argv[i], "-waves") == 0) {
      if (i == argc - 1) {
        std::cout << "Missing wave file argument." << std::endl;
        return -1;
      }
      // Skip over the wave file argument for the loop.
      wave_file = argv[i + 1];
      i++;
    } else {
      // Copy over all other arguments.
      pruned_args.push_back(argv[i]);
    }
  }

  if (!wave_file.empty()) {
    std::cout << "Reading wave file..." << std::endl;
    if (!sv::Workspace::Get().ReadWaves(wave_file)) {
      std::cout << "Problem reading wave file." << std::endl;
      return -1;
    }
  }

  // Parse the design using the remaining command line arguments
  if (pruned_args.size() > 1) {
    // TODO: Run in parallel threads with wave parsing?
    std::cout << "Parsing design files..." << std::endl;
    if (!sv::Workspace::Get().ParseDesign(
            pruned_args.size(),
            const_cast<const char **>(pruned_args.data()))) {
      // This function prints plenty of errors if it fails.
      return -1;
    }
  }
  // Start the UI and the event loop.
  sv::UI ui;
  ui.EventLoop();

  return 0;
}
