# These trio of files implement a workaround for LLVM bug 39427

set(ASM_DIALECT "-LLVMWAR")
set(CMAKE_ASM${ASM_DIALECT}_COMPILER_INIT "g++")
include(CMakeDetermineASMCompiler)
set(ASM_DIALECT)
