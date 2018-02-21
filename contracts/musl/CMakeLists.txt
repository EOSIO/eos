file(GLOB CRYPT_SOURCES  "upstream/src/crypt/*.c")
file(GLOB CTYPE_SOURCES  "upstream/src/ctype/*.c")
file(GLOB ENV_SOURCES  "upstream/src/env/*.c")
file(GLOB ERRNO_SOURCES  "upstream/src/errno/*.c")
file(GLOB EXIT_SOURCES  "upstream/src/exit/*.c")
file(GLOB LOCALE_SOURCES "upstream/src/locale/*.c")
file(GLOB MALLOC_SOURCES  "upstream/src/malloc/*.c")
file(GLOB MBYTE_SOURCES  "upstream/src/multibyte/*.c")
file(GLOB MISC_SOURCES "upstream/src/misc/*.c")
file(GLOB SEARCH_SOURCES "upstream/src/search/*.c")
file(GLOB STDIO_SOURCES "upstream/src/stdio/*.c")
file(GLOB STDLIB_SOURCES "upstream/src/stdlib/*.c")
file(GLOB STRING_SOURCES "upstream/src/string/*.c")
file(GLOB TIME_SOURCES "upstream/src/time/*.c")
file(GLOB THREAD_SOURCES "upstream/src/thread/*.c") #only for __lock __unlock
set(INTERNAL_SOURCES upstream/src/internal/intscan.c upstream/src/internal/shgetc.c upstream/src/internal/libc.c)

add_wast_library(TARGET libc
  NOWARNINGS
  SOURCE_FILES ${CRYPT_SOURCES} ${CTYPE_SOURCES} ${ENV_SOURCES} ${ERRNO_SOURCES} ${EXIT_SOURCES} ${INTERNAL_SOURCES} ${LOCALE_SOURCES} ${MBYTE_SOURCES}
  ${MISC_SOURCES} ${SEARCH_SOURCES} ${STDIO_SOURCES} ${STDLIB_SOURCES} ${STRING_SOURCES} ${TIME_SOURCES} ${THREAD_SOURCES}
  INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts/musl/upstream/include
                  ${CMAKE_SOURCE_DIR}/contracts/musl/upstream/src/internal
                  ${CMAKE_SOURCE_DIR}/contracts/musl/upstream/arch/eos
                  ${CMAKE_SOURCE_DIR}/contracts/
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
