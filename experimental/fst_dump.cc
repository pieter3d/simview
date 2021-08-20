#include "external/abseil-cpp/absl/strings/str_format.h"
#include "fstapi.h"
#include <iostream>

int main(int argc, char *argv[]) {

  if (argc != 2) {
    std::cout << "Specify an FST file." << std::endl;
    return -1;
  }

  void *r = fstReaderOpen(argv[1]);
  if (r == nullptr) {
    std::cout << "Problem opening " << argv[1] << std::endl;
    return -1;
  }

  std::cout << absl::StrFormat("timescale: %d\n", fstReaderGetTimescale(r));

  int indent = 0;
  fstHier *h;
  while ((h = fstReaderIterateHier(r))) {
    switch (h->htyp) {
    case FST_HT_SCOPE: {
      std::string name(h->u.scope.name, h->u.scope.name_length);
      std::string comp(h->u.scope.component, h->u.scope.component_length);
      std::string indent_str(indent,' ');
      std::cout << absl::StrFormat("%s[%s] - %d\n", indent_str, name, h->u.scope.typ);
      indent+=2;
                       }
      break;
    case FST_HT_UPSCOPE: 
      indent-=2;
      break;
    case FST_HT_VAR: {
      std::string indent_str(indent,' ');
      std::string name(h->u.var.name, h->u.var.name_length);
      std::string scope(fstReaderGetCurrentFlatScope(r), fstReaderGetCurrentScopeLen(r));
      std::cout << absl::StrFormat("%s%s(%d)\n", indent_str, name, h->u.var.direction);
      break;
    }
    default: std::cout << "Unknown thing\n"; break;
    }
  }
  std::cout << absl::StrFormat("Total # things: %d\n", fstReaderGetVarCount(r));

  fstReaderClose(r);
  return 0;
}
