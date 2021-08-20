add_executable(fst_dump
  fst_dump.cc
  )
target_link_libraries(fst_dump
  PRIVATE
    absl::str_format
    fst
)
set_target_properties(fst_dump PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
