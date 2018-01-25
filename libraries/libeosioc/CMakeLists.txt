INCLUDE_DIRECTORIES(include)

set(HEADERS
    include/eosio/libeosioc/endpoints.hpp
    include/eosio/libeosioc/remote.hpp
    include/eosio/libeosioc/wallet.hpp
    include/eosio/libeosioc/peer.hpp)

add_library(eosioc
    ${HEADERS}
    remote.cpp
    wallet.cpp
    peer.cpp
    )
target_link_libraries(eosioc fc eosio_chain)
target_include_directories(eosioc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_subdirectory(test)

install(TARGETS eosioc DESTINATION lib)
install(FILES ${HEADERS} DESTINATION "include/eosio/eosioclient")
