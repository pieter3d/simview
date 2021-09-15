# Enable all warnings as errors, except sign compare and unused params
set(CMAKE_CXX_FLAGS "-Werror \
                     -Wall \
                     -Wextra \
                     -Wno-sign-compare \
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
  fst_wave_data.cc
  main.cc
  panel.cc
  radix.cc
  signal_tree_item.cc
  source_panel.cc
  text_input.cc
  tree_data.cc
  tree_panel.cc
  uhdm_utils.cc
  ui.cc
  utils.cc
  vcd_tokenizer.cc
  vcd_wave_data.cc
  wave_data.cc
  wavedata_tree_item.cc
  wavedata_tree_panel.cc
  wave_signals_panel.cc
  waves_panel.cc
  workspace.cc
)
target_include_directories(simview SYSTEM PRIVATE ${CURSES_INCLUDE_DIR})
target_link_libraries(simview PRIVATE
  simple_tokenizer
  absl::str_format
  absl::time
  fst
  surelog
  ${CURSES_LIBRARIES}
)
set_target_properties(simview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
