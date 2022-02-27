# These trio of files implement a workaround for LLVM bug 39427

set(ASM_DIALECT "-LLVMWAR")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT "clang++")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT "g++")
endif()
include(CMakeDetermineASMCompiler)
set(ASM_DIALECT)
