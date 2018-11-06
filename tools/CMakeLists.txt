configure_file(${CMAKE_CURRENT_SOURCE_DIR}/llvm-gcov.sh ${CMAKE_CURRENT_BINARY_DIR}/llvm-gcov.sh COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ctestwrapper.sh ${CMAKE_CURRENT_BINARY_DIR}/ctestwrapper.sh COPYONLY)

add_executable( print_floats print_floats.cpp )
target_include_directories( print_floats PRIVATE ${Boost_INCLUDE_DIR} )
target_link_libraries( print_floats PRIVATE ${Boost_LIBRARIES} )
