set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

add_executable(blockvault_unittests main.cpp  postgres_backend_tests.cpp compressor_test.cpp)
target_link_libraries(blockvault_unittests blockvault Boost::unit_test_framework)

execute_process(COMMAND ${CMAKE_SOURCE_DIR}/scripts/postgres_control.sh status OUTPUT_VARIABLE HAS_POSTGRES_SERVER)

if (HAS_POSTGRES_SERVER)
   configure_file(test.sh ${CMAKE_CURRENT_BINARY_DIR}/test.sh @ONLY)
   add_test(NAME blockvault_unittests COMMAND test.sh)
endif()
