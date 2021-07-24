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
link_directories(/usr/local/lib/surelog)

find_library(SURELOG_LIBRARIES NAMES surelog
  PATH_SUFFIXES surelog) 
find_library(FLATBUFFERS_LIBRARIES NAMES flatbuffers
  PATH_SUFFIXES surelog) 
find_library(ANTLR_LIBRARIES NAMES antlr4-runtime
  PATH_SUFFIXES surelog) 
find_library(UHDM_LIBRARIES NAMES uhdm
  PATH_SUFFIXES uhdm) 
find_library(CAPNP_LIBRARIES NAMES capnp
  PATH_SUFFIXES uhdm) 
find_library(KJ_LIBRARIES NAMES kj
  PATH_SUFFIXES uhdm) 
find_path(SURELOG_INCLUDE_DIR "surelog/surelog.h")

set(ALL_SURELOG_LIBS
  ${SURELOG_LIBRARIES}
  ${UHDM_LIBRARIES}
  ${CAPNP_LIBRARIES}
  ${KJ_LIBRARIES}
  ${FLATBUFFERS_LIBRARIES}
  ${ANTLR_LIBRARIES}
  Threads::Threads
)

add_executable(simview
  hierarchy.cc
  source.cc
  waves.cc
  ui.cc
  main.cc
)
target_include_directories(simview SYSTEM PRIVATE ${CURSES_INCLUDE_DIR})
target_include_directories(simview PRIVATE
  ${SURELOG_INCLUDE_DIR}/surelog
  ${SURELOG_INCLUDE_DIR}/uhdm
  ${SURELOG_INCLUDE_DIR}/uhdm/headers
  ${SURELOG_INCLUDE_DIR}/uhdm/include
)
target_link_libraries(simview PUBLIC
  ${CURSES_LIBRARIES}
  ${ALL_SURELOG_LIBS}
)
set_target_properties(simview PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
