
add_executable(blockvault_unittests main.cpp log_store_tests.cpp ../log_store.cpp)
target_link_libraries(blockvault_unittests nuraft fc Boost::unit_test_framework)

add_test(NAME blockvault_tests COMMAND blockvault_unittests)
