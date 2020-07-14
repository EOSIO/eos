if (OPENSSL_FOUND)

# Test transport integration
file (GLOB SOURCE integration.cpp)

init_target (test_transport)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
link_openssl()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test transport asio timers
file (GLOB SOURCE asio/timers.cpp)

init_target (test_transport_asio_timers)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
link_openssl()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test transport asio security
file (GLOB SOURCE asio/security.cpp)

init_target (test_transport_asio_security)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
link_openssl()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

endif()

# Test transport iostream base
file (GLOB SOURCE iostream/base.cpp)

init_target (test_transport_iostream_base)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test transport iostream endpoint
file (GLOB SOURCE iostream/endpoint.cpp)

init_target (test_transport_iostream_endpoint)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test transport iostream connection
file (GLOB SOURCE iostream/connection.cpp)

init_target (test_transport_iostream_connection)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

# Test transport asio base
file (GLOB SOURCE asio/base.cpp)

init_target (test_transport_asio_base)
build_test (${TARGET_NAME} ${SOURCE})
link_boost ()
final_target ()
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")


