add_executable(system system.cpp)
target_compile_options(system PUBLIC -no-missing-ricardian-clause)
set_target_properties(system PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
