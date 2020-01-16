file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/core DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schedule_test.py ${CMAKE_CURRENT_BINARY_DIR}/schedule_test.py COPYONLY)
add_test(NAME producer_schedule_lstest COMMAND python3 lstests/schedule_test.py --debug --monochrome --cluster-id 1 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST producer_schedule_lstest PROPERTY LABELS ls_tests)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/catchup_test.py ${CMAKE_CURRENT_BINARY_DIR}/catchup_test.py COPYONLY)
add_test(NAME catchup_lstest COMMAND python3 lstests/catchup_test.py --debug --monochrome --cluster-id 2 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST catchup_lstest PROPERTY LABELS ls_tests)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/db_guard_test.py ${CMAKE_CURRENT_BINARY_DIR}/db_guard_test.py COPYONLY)
add_test(NAME db_guard_lstest COMMAND python3 lstests/db_guard_test.py --debug --monochrome --cluster-id 3 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST db_guard_lstest PROPERTY LABELS ls_tests)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fork_test.py ${CMAKE_CURRENT_BINARY_DIR}/fork_test.py COPYONLY)
add_test(NAME fork_lstest COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 4 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_1 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 5 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_1 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_2 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 6 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_2 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_3 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 7 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_3 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_4 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 8 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_4 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_5 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 9 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_5 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_6 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 10 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_6 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_7 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 11 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_7 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_8 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 12 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_8 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_9 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 13 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_9 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_10 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 14 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_10 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_11 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 15 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_11 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_12 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 16 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_12 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_13 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 17 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_13 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_14 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 18 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_14 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_15 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 19 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_15 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_16 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 20 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_16 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_17 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 21 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_17 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_18 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 22 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_18 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_19 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 23 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_19 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_20 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 24 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_20 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_21 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 25 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_21 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_22 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 26 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_22 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_23 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 27 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_23 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_24 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 28 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_24 PROPERTY LABELS ls_tests)

add_test(NAME fork_lstest_25 COMMAND python3 lstests/fork_test.py --debug --monochrome --cluster-id 29 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST fork_lstest_25 PROPERTY LABELS ls_tests)
