set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_C_COMPILER /usr/local/bin/clang)
set(CMAKE_CXX_COMPILER /usr/local/bin/clang++)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES /usr/local/include/c++/v1 /usr/local/include /usr/include)

set(CMAKE_CXX_FLAGS_INIT "-nostdinc++")

set(CMAKE_EXE_LINKER_FLAGS_INIT "-stdlib=libc++ -nostdlib++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-stdlib=libc++ -nostdlib++")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-stdlib=libc++ -nostdlib++")

set(CMAKE_CXX_STANDARD_LIBRARIES "/usr/local/lib/libc++.a /usr/local/lib/libc++abi.a")
