file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/core DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/catchup_test.py ${CMAKE_CURRENT_BINARY_DIR}/catchup_test.py COPYONLY)
add_test(NAME catchup_lstest COMMAND python3 lstests/catchup_test.py --debug --monochrome --cluster-id 2 --wdir . WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_property(TEST catchup_lstest PROPERTY LABELS ls_tests)
