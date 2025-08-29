# Enable all warnings as errors, except sign compare and unused params
set(CMAKE_CXX_FLAGS "-Werror \
                     -Wall \
                     -Wextra \
                     -Wno-sign-compare \
                     -Wno-unused-parameter")

# Full debug symbols for DEBUG
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

find_package(Curses REQUIRED ncursesw)
find_package(Threads REQUIRED)
find_package(Surelog REQUIRED)
find_package(UHDM REQUIRED)
find_package(ZLIB REQUIRED)  # needed for fst and optionally used by surelog

add_library(simple_tokenizer simple_tokenizer.cc)
target_link_libraries(simple_tokenizer PUBLIC absl::flat_hash_map)
simview_add_test(simple_tokenizer_test simple_tokenizer_test.cc)
target_link_libraries(simple_tokenizer_test PRIVATE simple_tokenizer)

add_library(source_buffer source_buffer.cc)
target_link_libraries(source_buffer PUBLIC absl::flat_hash_map absl::btree)
simview_add_test(source_buffer_test source_buffer_test.cc)
target_link_libraries(source_buffer_test PRIVATE source_buffer)

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
  slang_utils.cc
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
target_include_directories(simview SYSTEM PRIVATE ${CURSES_INCLUDE_DIR} ${UHDM_INCLUDE_DIR} ${SURELOG_INCLUDE_DIR})
target_link_libraries(simview PRIVATE
  simple_tokenizer
  source_buffer
  absl::str_format
  absl::time
  absl::flat_hash_map
  fst
  slang::slang
  surelog::surelog
  uhdm::uhdm
  ${CURSES_LIBRARIES}
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_custom_command(TARGET simview POST_BUILD
    COMMAND ${CMAKE_STRIP} $<TARGET_FILE:simview>
    COMMENT "Stripping simview")
endif()


# TODO: Remove eventually.
add_executable(demo demo.cc)
target_link_libraries(demo PRIVATE
  absl::str_format
  slang::slang)
