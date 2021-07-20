# Enable all warnings as errors, except sign compare.
set(CMAKE_CXX_FLAGS "-Werror \
                     -Wall \
                     -Wextra \
                     -Wno-sign-compare \
                     -Wno-unused-parameter")

# Surelog should be checked out and built.
message("SURELOG_ROOT: " $ENV{SURELOG_ROOT})
if (NOT DEFINED ENV{SURELOG_ROOT})
  message(FATAL_ERROR "SURELOG_ROOT is not set.")
endif()

# NCurses for the TUI
find_package(Curses REQUIRED)

find_library(SURELOG_LIBRARIES NAMES surelog HINTS $ENV{SURELOG_ROOT}/build/lib) 

add_library(simview_lib STATIC
  ui.cc
)
target_include_directories(simview_lib SYSTEM PRIVATE ${CURSES_INCLUDE_DIR})
target_include_directories(simview_lib PRIVATE
  $ENV{SURELOG_ROOT}/src
  $ENV{SURELOG_ROOT}/third_party/UHDM
  $ENV{SURELOG_ROOT}/third_party/UHDM/headers
  $ENV{SURELOG_ROOT}/third_party/UHDM/include)
target_link_libraries(simview_lib PUBLIC absl::str_format)
target_link_libraries(simview_lib PUBLIC ${SURELOG_LIBRARIES})
target_link_libraries(simview_lib PUBLIC ${CURSES_LIBRARIES})

add_executable(simview main.cc)
target_link_libraries(simview PRIVATE simview_lib)
set_target_properties(simview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
