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
  fstHandle h_reset;
  fstHandle h_wr_ptr;
  fstHandle h_int;
  while ((h = fstReaderIterateHier(r))) {
    switch (h->htyp) {
    case FST_HT_SCOPE: {
      std::string name(h->u.scope.name, h->u.scope.name_length);
      std::string comp(h->u.scope.component, h->u.scope.component_length);
      std::string indent_str(indent, ' ');
      std::cout << absl::StrFormat("%s[%s] - %d\n", indent_str, name,
                                   h->u.scope.typ);
      indent += 2;
    } break;
    case FST_HT_UPSCOPE: indent -= 2; break;
    case FST_HT_VAR: {
      std::string indent_str(indent, ' ');
      std::string name(h->u.var.name, h->u.var.name_length);
      std::string scope(fstReaderGetCurrentFlatScope(r),
                        fstReaderGetCurrentScopeLen(r));
      std::cout << absl::StrFormat("%s%s(%d)%c len = %d, type = %d\n",
                                   indent_str, name, h->u.var.direction,
                                   h->u.var.is_alias ? 'a' : ' ',
                                   h->u.var.length, h->u.var.typ);
      if (!h->u.var.is_alias) {
        if (name == "rd_arst_n") h_reset = h->u.var.handle;
        if (name == "wr_ptr") h_wr_ptr = h->u.var.handle;
        if (name == "silly_int") h_int = h->u.var.handle;
      }
      break;
    }
    default: std::cout << "Unknown thing\n"; break;
    }
  }
  std::cout << absl::StrFormat("Total # things: %d\n", fstReaderGetVarCount(r));
  std::cout << absl::StrFormat("Time range: %ld -> %ld\n",
                               fstReaderGetStartTime(r),
                               fstReaderGetEndTime(r));
  std::cout << absl::StrFormat("Time zero: %ld\n", fstReaderGetTimezero(r));
  char buf[100];
  memset(buf, 0, 100);
  fstReaderGetValueFromHandleAtTime(r, 1, h_reset, buf);
  printf("rst at 1: %s\n", buf);
  fstReaderGetValueFromHandleAtTime(r, 100000, h_reset, buf);
  printf("rst at 100000: %s\n", buf);
  fstReaderGetValueFromHandleAtTime(r, 100000, h_wr_ptr, buf);
  printf("wr_ptr at 100000: %s\n", buf);
  fstReaderGetValueFromHandleAtTime(r, 1000, h_int, buf);
  printf("silly_int at 1000: %s\n", buf);

  fstReaderSetLimitTimeRange(r, 80'000, 400'000);
  fstReaderSetFacProcessMask(r, h_reset);
  fstReaderSetFacProcessMask(r, h_wr_ptr);
  fstReaderIterBlocks(
      r,
      +[](void *user_callback_data_pointer, uint64_t time, fstHandle facidx,
          const unsigned char *value) {
        auto rst = static_cast<fstHandle *>(user_callback_data_pointer);
        std::string signal = facidx == *rst ? "rd_arst_n" : "wr_ptr";
        printf("%s: %s @ %ld\n", signal.c_str(), value, time);
      },
      &h_reset, nullptr);

  fstReaderClose(r);
  return 0;
}
