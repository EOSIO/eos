set(SOURCES abi_generator.cpp)

set(CMAKE_CXX_FLAGS "-fno-rtti")

# Find an installed build of LLVM
find_package(LLVM 4.0 REQUIRED CONFIG)

# Include the LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})

add_library( abi_generator SHARED ${SOURCES})
target_link_libraries( abi_generator PUBLIC fc eos_types)

install( TARGETS
   abi_generator
   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
