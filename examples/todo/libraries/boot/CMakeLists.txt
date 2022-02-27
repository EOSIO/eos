add_executable(boot boot.cpp)
target_compile_options(boot PUBLIC -no-missing-ricardian-clause)
set_target_properties(boot PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
