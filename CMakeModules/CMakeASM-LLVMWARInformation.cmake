# These trio of files implement a workaround for LLVM bug 39427

set(ASM_DIALECT "-LLVMWAR")
set(CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS llvmwar)

set(CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "g++ -x c++ -O3 --std=c++11 <DEFINES> <INCLUDES> <FLAGS> -c -o <OBJECT> <SOURCE>")

include(CMakeASMInformation)
set(ASM_DIALECT)
