set(SOURCES abi_generator.cpp)
#execute_process(
#  COMMAND llvm-config --cxxflags
#  RESULT_VARIABLE LLVM_CONFIG_OK
#  OUTPUT_VARIABLE LLVM_CXXFLAGS
#)

set(CMAKE_CXX_FLAGS "-fno-rtti")

include_directories("/usr/lib/llvm-4.0/include")
add_library( abi_generator SHARED ${SOURCES})
#add_compile_options(-fno-rtti)
target_link_libraries( abi_generator 
  PUBLIC fc eos_types)


install( TARGETS
   abi_generator

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
