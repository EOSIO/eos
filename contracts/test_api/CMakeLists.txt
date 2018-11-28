set( CMAKE_CXX_STANDARD 17 )

include_directories( ${MYPROJECT_DIR}../ )

add_contract( test_api test_api test_api.cpp )
