
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.bios/bin/eosio.bios.abi
          ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.bios/bin/eosio.bios.wasm
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/contracts/eosio.bios)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.boot/bin/eosio.boot.abi
          ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.boot/bin/eosio.boot.wasm
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/contracts/eosio.boot)

configure_file(boot.sh boot.sh COPYONLY)
configure_file(config.ini config.ini COPYONLY)
configure_file(wait-for-it.sh wait-for-it.sh COPYONLY)

execute_process(COMMAND curl http://169.254.169.254/latest/meta-data/public-ipv4 OUTPUT_VARIABLE DOCKER_REPO)

add_custom_target(test_docker_image ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/bin ${CMAKE_CURRENT_BINARY_DIR}/bin
    COMMAND ${Docker_EXECUTABLE} build --tag ${DOCKER_REPO}/eos-boxed --file ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)

add_dependencies(test_docker_image ${NODE_EXECUTABLE_NAME} ${RODEOS_EXECUTABLE_NAME} ${CLI_CLIENT_EXECUTABLE_NAME} ${KEY_STORE_EXECUTABLE_NAME})