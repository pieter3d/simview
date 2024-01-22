# Lets keep things modern
cmake_minimum_required(VERSION 3.14)

project(SimView)

# Support YouCompleteMe for vim.
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")

# Modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure default build mode is RELEASE.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
# Full debug symbols for DEBUG, optimizations for release.
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")

# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if(EXISTS "${LOC_PATH}")
  message(FATAL_ERROR 
    "You cannot build in a source directory \
    (or any directory with a CMakeLists.txt \
    file). Please make a build subdirectory. \
    Feel free to remove CMakeCache.txt and CMakeFiles."
  )
endif()

# Ninja builder doesn't enable color prints since it doesn't 
# direct compiler output direct to stdout.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-fdiagnostics-color=always)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fcolor-diagnostics)
endif()

# Helper macros are defined in a separate file.
include(macros.cmake)

# Testing is good!
enable_testing()

# All include is relative to project root, or local dir.
include_directories(${PROJECT_SOURCE_DIR})

# Add all external projects
add_subdirectory(external)

# Add all source
add_subdirectory(src)
