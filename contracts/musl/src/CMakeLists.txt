file(GLOB CRYPT_SOURCES  "crypt/*.c")
file(GLOB CTYPE_SOURCES  "ctype/*.c")
file(GLOB ENV_SOURCES  "env/*.c")
file(GLOB ERRNO_SOURCES  "errno/*.c")
file(GLOB EXIT_SOURCES  "exit/*.c")
file(GLOB LOCALE_SOURCES "locale/*.c")
file(GLOB MBYTE_SOURCES  "multibyte/*.c")
file(GLOB SEARCH_SOURCES "search/*.c")
file(GLOB STDIO_SOURCES "stdio/*.c")
file(GLOB STDLIB_SOURCES "stdlib/*.c")
file(GLOB STRING_SOURCES "string/*.c")
file(GLOB TIME_SOURCES "time/*.c")
file(GLOB THREAD_SOURCES "thread/*.c") #only for __lock __unlock
file(GLOB INTERNAL_SOURCES internal/intscan.c internal/shgetc.c internal/libc.c)

add_wast_library(TARGET libc
  SOURCE_FILES ${CRYPT_SOURCES} ${CTYPE_SOURCES} ${ENV_SOURCES} ${ERRNO_SOURCES} ${EXIT_SOURCES} ${INTERNAL_SOURCES} ${LOCALE_SOURCES} ${MBYTE_SOURCES}
  ${SEARCH_SOURCES} ${STDIO_SOURCES} ${STDLIB_SOURCES} ${STRING_SOURCES} ${TIME_SOURCES} ${THREAD_SOURCES}
  INCLUDE_FOLDERS ${CMAKE_SOURCE_DIR}/contracts/musl/include ${CMAKE_SOURCE_DIR}/contracts/musl/src/internal ${CMAKE_SOURCE_DIR}/contracts/musl/arch/eos ${CMAKE_SOURCE_DIR}/contracts/
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
