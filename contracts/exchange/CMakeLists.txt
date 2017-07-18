file(GLOB SOURCE_FILES "*.cpp")
add_wast_target(exchange "${SOURCE_FILES}" "${CMAKE_SOURCE_DIR}/contracts" ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies( exchange currency )
