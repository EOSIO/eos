
add_executable(sample_server
  sample_server.cc
)

target_link_libraries(sample_server
  PRIVATE
    ${PROJECT_NAME}::pull
)

add_executable(sample_server_multi
  sample_server_multi.cc
)

target_link_libraries(sample_server_multi
  PRIVATE
    ${PROJECT_NAME}::pull
)

add_executable(sample_server_auth
  sample_server_auth.cc
)

target_link_libraries(sample_server_auth
  PRIVATE
     ${PROJECT_NAME}::pull
)

find_package(CURL)

if(CURL_FOUND)
  add_executable(prometheus_pull_integration_test
    integration_test.cc
  )

  target_link_libraries(prometheus_pull_integration_test
    PRIVATE
      ${PROJECT_NAME}::pull
      CURL::libcurl
      GTest::gmock_main
  )

  add_test(
    NAME prometheus_pull_integration_test
    COMMAND prometheus_pull_integration_test
  )
endif()
