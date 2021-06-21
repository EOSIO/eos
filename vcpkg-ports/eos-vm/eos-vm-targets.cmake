
add_library(eos-vm INTERFACE)
target_include_directories(eos-vm
                           INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../../include)

# ignore the C++17 register warning until clean up
target_compile_options( eos-vm INTERFACE "-Wno-register" )
target_compile_definitions(eos-vm INTERFACE -DEOS_VM_SOFTFLOAT)
target_link_libraries(eos-vm INTERFACE softfloat)