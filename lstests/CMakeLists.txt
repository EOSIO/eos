
add_test(NAME producer_schedule_lstest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/schedule_test.py --mono --cluster-id 1)
set_property(TEST producer_schedule_lstest PROPERTY LABELS ls_tests)

add_test(NAME catchup_lstest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/catchup_test.py --mono --cluster-id 2)
set_property(TEST catchup_lstest PROPERTY LABELS ls_tests)

add_test(NAME db_guard_lstest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/db_guard_test.py --mono --cluster-id 3)
set_property(TEST db_guard_lstest PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/fork_test.py --mono --cluster-id 4)
set_property(TEST fork_lstest PROPERTY LABELS ls_tests)

