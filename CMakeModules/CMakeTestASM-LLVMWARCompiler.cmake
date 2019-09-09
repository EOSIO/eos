# These trio of files implement a workaround for LLVM bug 39427

set(ASM_DIALECT "-LLVMWAR")
include(CMakeTestASMCompiler)
set(ASM_DIALECT)
