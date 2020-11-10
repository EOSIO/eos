add_executable(blockvault_unittests main.cpp  postgres_backend_tests.cpp compressor_test.cpp)
target_link_libraries(blockvault_unittests blockvault Boost::unit_test_framework)

configure_file(test.sh ${CMAKE_CURRENT_BINARY_DIR}/test.sh COPYONLY)

find_program(DOCKER docker)
if (DOCKER)
   add_test(NAME blockvault_unittests COMMAND test.sh)
endif(DOCKER)