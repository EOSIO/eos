add_executable(todo_test todo_test.cpp)
add_dependencies(todo_test todo_contract todo_filter)
target_link_libraries(todo_test -ftester -stack-size=65536)
target_compile_options(todo_test PUBLIC -ftester -Os -no-missing-ricardian-clause)
set_target_properties(todo_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ROOT_BINARY})
target_include_directories(todo_test PUBLIC ${ROOT_SOURCE}/libraries)

add_custom_command(
    TARGET todo_test
    # On Visual Studio Generators, run before any other rules are executed within the target. On other generators, run just before PRE_LINK commands
    PRE_BUILD
    COMMAND echo -e "\texecuting a PRE_BUILD command"
    COMMENT "This command will be executed before building bar"
    VERBATIM # to support \t for example
)

add_custom_command(
  OUTPUT todo_test
  DEPENDS
    boot
    system
  WORKING_DIRECTORY ${ROOT_BINARY}
  POST_BUILD COMMAND ${EOSIO_TESTER_PATH} todo_test.wasm
)
