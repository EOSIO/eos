
add_executable(prometheus_pull_test
  exposer_test.cc
)

target_link_libraries(prometheus_pull_test
  PRIVATE
    ${PROJECT_NAME}::pull
    GTest::gmock_main
)

add_test(
  NAME prometheus_pull_test
  COMMAND prometheus_pull_test
)
