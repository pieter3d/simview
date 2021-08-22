# Enable all warnings as errors, except sign compare.
# Additional warning suppressions due to coding style in Surelog & UHDM
set(CMAKE_CXX_FLAGS "-Werror \
                     -Wall \
                     -Wextra \
                     -Wno-sign-compare \
                     -Wno-final-dtor-non-final-class \
                     -Wno-inconsistent-missing-override \
                     -Wno-unused-parameter")

find_package(Curses REQUIRED)
find_package(Threads REQUIRED)
find_package(Surelog REQUIRED)

add_library(simple_tokenizer simple_tokenizer.cc)
simview_add_test(simple_tokenizer_test simple_tokenizer_test.cc)
target_link_libraries(simple_tokenizer_test PRIVATE simple_tokenizer)

add_executable(simview
  color.cc
  design_tree_item.cc
  design_tree_panel.cc
  tree_data.cc
  tree_panel.cc
  main.cc
  panel.cc
  source.cc
  text_input.cc
  ui.cc
  utils.cc
  waves.cc
  workspace.cc
)
target_include_directories(simview SYSTEM PRIVATE ${CURSES_INCLUDE_DIR})
target_link_libraries(simview PRIVATE
  simple_tokenizer
  absl::str_format
  absl::time
  surelog
  ${CURSES_LIBRARIES}
)
set_target_properties(simview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
