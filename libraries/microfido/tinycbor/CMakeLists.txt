
set(CMAKE_C_STANDARD 99)

add_library(tinycbor
   upstream/src/cborerrorstrings.c
   upstream/src/cborencoder.c
   upstream/src/cborencoder_close_container_checked.c
   upstream/src/cborparser.c
   upstream/src/cborpretty.c
   upstream/src/cborparser_dup_string.c
   upstream/src/cborpretty_stdio.c
   upstream/src/cbortojson.c
   upstream/src/cborvalidation.c
)

target_include_directories(tinycbor PUBLIC upstream/src)
