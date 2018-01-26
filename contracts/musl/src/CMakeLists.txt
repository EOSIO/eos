#file(GLOB CRYPT_SOURCES  "crypt/*.c")
file(GLOB CTYPE_SOURCES  "ctype/*.c")
file(GLOB ERRNO_SOURCES  "errno/*.c")
file(GLOB MBYTE_SOURCES  "multibyte/*.c")
file(GLOB SEARCH_SOURCES "search/*.c")
file(GLOB STDLIB_SOURCES "stdlib/*.c")
file(GLOB STRING_SOURCES "string/*.c")

add_wast_library(TARGET libc
  SOURCE_FILES ${CRYPT_SOURCES} ${CTYPE_SOURCES} ${CRYPT_SOURCES} ${ERRNO_SOURCES} ${MBYTE_SOURCES} ${SEARCH_SOURCES} ${STDLIB_SOURCES} ${STRING_SOURCES}
  INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts/musl/include ${CMAKE_SOURCE_DIR}/contracts/musl/src/internal ${CMAKE_SOURCE_DIR}/contracts/musl/arch/eos
)
