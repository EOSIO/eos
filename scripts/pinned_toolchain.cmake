set(OPT_PATH @)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER ${OPT_PATH}/clang8/bin/clang)
set(CMAKE_CXX_COMPILER ${OPT_PATH}/clang8/bin/clang++)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${OPT_PATH}/clang8/include/c++/v1 /usr/local/include /usr/include)

set(CMAKE_C_FLAGS_INIT "-D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie")
set(CMAKE_CXX_FLAGS_INIT "-nostdinc++ -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fpie")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-stdlib=libc++ -nostdlib++ -pie")
if(NOT APPLE)
   string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -Wl,-z,relro,-z,now")
endif()

set(CMAKE_SHARED_LINKER_FLAGS_INIT "-stdlib=libc++ -nostdlib++")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-stdlib=libc++ -nostdlib++")

set(CMAKE_CXX_STANDARD_LIBRARIES "${OPT_PATH}/clang8/lib/libc++.a ${OPT_PATH}/clang8/lib/libc++abi.a")
