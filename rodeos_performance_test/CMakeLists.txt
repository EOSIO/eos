
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.bios/bin/eosio.bios.abi
          ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.bios/bin/eosio.bios.wasm
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/contracts/eosio.bios)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.boot/bin/eosio.boot.abi
          ${CMAKE_CURRENT_SOURCE_DIR}/../contracts/contracts/eosio.boot/bin/eosio.boot.wasm
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/contracts/eosio.boot)

file(COPY contracts/kv_contract DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/contracts)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../unittests/contracts/eosio.token DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/unittests/contracts)

configure_file(boot.sh boot.sh COPYONLY)
configure_file(config.ini config.ini COPYONLY)
configure_file(wait-for-it.sh wait-for-it.sh COPYONLY)

if (NOT MANAGER_IP)
    execute_process(COMMAND curl --connect-timeout 1 -s http://169.254.169.254/latest/meta-data/public-ipv4 OUTPUT_VARIABLE MANAGER_IP)
endif()

if ("${MANAGER_IP}")
    set(docker_tag_option --tag ${MANAGER_IP}:5443/eos-boxed)
endif()

add_custom_target(test_docker_image ALL
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/nodeos ${CMAKE_CURRENT_BINARY_DIR}/bin/nodeos
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/rodeos ${CMAKE_CURRENT_BINARY_DIR}/bin/rodeos
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/cleos ${CMAKE_CURRENT_BINARY_DIR}/bin/cleos
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/bin/keosd ${CMAKE_CURRENT_BINARY_DIR}/bin/keosd
    COMMAND ${Docker_EXECUTABLE} build --build-arg BUILD_DIR=${CMAKE_BINARY_DIR} 
                                       --tag eos-boxed 
                                       ${docker_tag_option}
                                       --file ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile 
                                       ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile)

add_dependencies(test_docker_image ${NODE_EXECUTABLE_NAME} ${RODEOS_EXECUTABLE_NAME} ${CLI_CLIENT_EXECUTABLE_NAME} ${KEY_STORE_EXECUTABLE_NAME})