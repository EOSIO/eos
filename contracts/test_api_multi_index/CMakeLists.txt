set( CMAKE_CXX_STANDARD 17 )

include_directories( ${Boost_INCLUDE_DIRS} )

add_contract( test_api_multi_index test_api_multi_index test_api_multi_index.cpp )
