find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(vcd_lexer vcd_lexer.l
  "${CMAKE_CURRENT_BINARY_DIR}/vcd_lexer.gen.cc"
  COMPILE_FLAGS "--header-file=vcd_lexer.gen.h"
)
bison_target(vcd_parser vcd_parser.y
  "${CMAKE_CURRENT_BINARY_DIR}/vcd_parser.gen.cc"
  COMPILE_FLAGS "--defines=vcd_parser.gen.h"
)
add_flex_bison_dependency(vcd_lexer vcd_parser)

add_library(vcdparse
  vcd_data.cc
  "${CMAKE_CURRENT_BINARY_DIR}/vcd_lexer.gen.cc"
  "${CMAKE_CURRENT_BINARY_DIR}/vcd_parser.gen.cc"
)
target_include_directories(vcdparse
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)
