file(GLOB ABI_FILES "*.abi")
add_wast_executable(TARGET exchange
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}"
  LIBRARIES libc++ libc eosiolib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)
add_dependencies( exchange currency )

add_executable(test_exchange test_exchange.cpp )
#bfp/lib/pack.c bfp/lib/posit.cpp bfp/lib/util.c bfp/lib/op2.c)
target_link_libraries( test_exchange fc )
target_include_directories( test_exchange PUBLIC fixed_point/include )
