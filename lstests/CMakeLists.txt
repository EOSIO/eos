file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/core DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

message("PYTHON3 is ", ${PYTHON3})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/schedule_test.py ${CMAKE_CURRENT_BINARY_DIR}/schedule_test.py COPYONLY)
add_test(NAME producer_schedule_lstest COMMAND ${PYTHON3} lstests/schedule_test.py --debug --mono --cluster-id 1 --wdir . WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST producer_schedule_lstest PROPERTY LABELS ls_tests)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/catchup_test.py ${CMAKE_CURRENT_BINARY_DIR}/catchup_test.py COPYONLY)
add_test(NAME catchup_lstest COMMAND ${PYTHON3} lstests/catchup_test.py --debug --mono --cluster-id 2 --wdir . WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST catchup_lstest PROPERTY LABELS ls_tests)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/db_guard_test.py ${CMAKE_CURRENT_BINARY_DIR}/db_guard_test.py COPYONLY)
add_test(NAME db_guard_lstest COMMAND ${PYTHON3} lstests/db_guard_test.py --debug --mono --cluster-id 3 --wdir . WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST db_guard_lstest PROPERTY LABELS ls_tests)

