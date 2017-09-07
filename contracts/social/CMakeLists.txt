file(GLOB SOURCE_FILES "*.cpp")
add_wast_target(social "${SOURCE_FILES}" "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_BINARY_DIR})