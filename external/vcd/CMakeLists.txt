find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(VCDScanner VCDScanner.l
  "${CMAKE_CURRENT_BINARY_DIR}/VCDScanner.cc"
  COMPILE_FLAGS "--header-file=VCDScanner.h"
)
bison_target(VCDParser VCDParser.ypp
  "${CMAKE_CURRENT_BINARY_DIR}/VCDParser.cc"
  COMPILE_FLAGS "--defines=VCDParser.h"
)
add_flex_bison_dependency(VCDScanner VCDParser)
add_library(VCDFileParser
  VCDFileParser.cc
  VCDFile.cc
  "${CMAKE_CURRENT_BINARY_DIR}/VCDScanner.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/VCDParser.cc"
)
target_include_directories(VCDFileParser
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
