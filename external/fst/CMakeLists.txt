find_package(ZLIB REQUIRED)

add_library(fst
  fstapi.c
  fastlz.c
  lz4.c
)
target_link_libraries(fst INTERFACE ${ZLIB_LIBRARY})
target_include_directories(fst PUBLIC .)
