add_executable(prometheus_pull_internal_test
  base64_test.cc
)

target_link_libraries(prometheus_pull_internal_test
  PRIVATE
    ${PROJECT_NAME}::pull_internal_headers
    GTest::gmock_main
)

add_test(
  NAME prometheus_pull_internal_test
  COMMAND prometheus_pull_internal_test
)
