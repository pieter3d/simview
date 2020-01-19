# Enable all warnings as errors, except sign compare.
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-sign-compare")

add_library(temp
  temp.cc
)
target_link_libraries(temp
  PRIVATE
  absl::str_format
)
