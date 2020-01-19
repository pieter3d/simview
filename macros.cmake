# Simplify testing definitions.
macro(simview_add_test Name)
  add_executable(${Name} ${ARGN})
  target_link_libraries(${Name} PRIVATE gtest_main)
  add_test(NAME ${Name} COMMAND ${Name})
endmacro()
