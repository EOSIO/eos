# Inspired by CheckAtomic.cmake from LLVM project:
# https://github.com/llvm/llvm-project/blob/master/llvm/cmake/modules/CheckAtomic.cmake
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

include(CheckCXXSourceCompiles)
include(CheckLibraryExists)

function(check_working_cxx_atomics varname)
  check_cxx_source_compiles("
#include <atomic>
#include <cstdint>
std::atomic<std::uint64_t> x(0);
int main() {
  std::uint64_t i = x.load(std::memory_order_relaxed);
  return static_cast<int>(i);
}
" ${varname})
endfunction()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  # First check if atomics work without the library.
  check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITHOUT_LIB)
  # If not, check if the library exists, and atomics work with it.
  if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
    check_library_exists(atomic __atomic_load_8 "" HAVE_CXX_LIBATOMIC)
    if(HAVE_CXX_LIBATOMIC)
      list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
      check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITH_LIB)
      if(NOT HAVE_CXX_ATOMICS_WITH_LIB)
        message(FATAL_ERROR "Host compiler must support 64-bit std::atomic!")
      endif()
    else()
      message(FATAL_ERROR "Host compiler appears to require libatomic for 64-bit operations, but cannot find it.")
    endif()
  endif()
endif()
