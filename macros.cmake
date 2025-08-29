# Simplify testing definitions.
macro(simview_add_test Name)
  add_executable(${Name} ${ARGN})
  target_link_libraries(${Name} PRIVATE GTest::gtest_main)
  gtest_discover_tests(${Name})
endmacro()
