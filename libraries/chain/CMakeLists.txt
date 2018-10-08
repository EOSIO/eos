configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/chain/core_symbol.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/eosio/chain/core_symbol.hpp)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/genesis_state_root_key.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp)

set(CHAINDB_DEFS "")
set(CHAINDB_INCS "")
set(CHAINDB_LIBS "")
set(CHAINDB_SRCS "")

if (BUILD_CHAINDB_MONGO)

    find_package(libmongoc-1.0 1.8)

    if (libmongoc-1.0_FOUND)
        find_package(libbsoncxx-static REQUIRED)
        message(STATUS "Found bsoncxx headers for chaindb: ${LIBBSONCXX_STATIC_INCLUDE_DIRS}")

        if ((LIBBSONCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBBSONCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBBSONCXX_STATIC_VERSION_MINOR LESS 2)))
            find_library(CHAINDB_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES}
                PATHS ${LIBBSONCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
        else()
            set(CHAINDB_LIBBSONCXX ${LIBBSONCXX_STATIC_LIBRARIES})
        endif()
        message(STATUS "Found bsoncxx library for chaindb: ${CHAINDB_LIBBSONCXX}")

        find_package(libmongocxx-static REQUIRED)
        message(STATUS "Found mongocxx headers for chaindb: ${LIBMONGOCXX_STATIC_INCLUDE_DIRS}")

        # mongo-cxx-driver 3.2 release altered LIBBSONCXX_LIBRARIES semantics. Instead of library names,
        #  it now hold library paths.
        if ((LIBMONGOCXX_STATIC_VERSION_MAJOR LESS 3) OR ((LIBMONGOCXX_STATIC_VERSION_MAJOR EQUAL 3) AND (LIBMONGOCXX_STATIC_VERSION_MINOR LESS 2)))
            find_library(CHAINDB_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES}
                PATHS ${LIBMONGOCXX_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
        else()
            set(CHAINDB_LIBMONGOCXX ${LIBMONGOCXX_STATIC_LIBRARIES})
        endif()
        message(STATUS "Found mongocxx library for chaindb: ${CHAINDB_LIBMONGOCXX}")

        set(CHAINDB_DEFS ${CHAINDB_DEFS} CYBERWAY_CHAINDB=1 ${LIBMONGOCXX_STATIC_DEFINITIONS} ${LIBBSONCXX_STATIC_DEFINITIONS})
        set(CHAINDB_INCS ${CHAINDB_INCS} ${BSON_INCLUDE_DIRS} ${LIBMONGOCXX_STATIC_INCLUDE_DIRS} ${LIBBSONCXX_STATIC_INCLUDE_DIRS})
        set(CHAINDB_LIBS ${CHAINDB_LIBS} ${CHAINDB_LIBMONGOCXX} ${CHAINDB_LIBBSONCXX} ${BSON_LIBRARIES})
        set(CHAINDB_SRCS ${CHAINDB_SRCS} chaindb/chaindb_mongo.cpp)

    else()
        message("Could NOT find MongoDB. chaindb with MongoDB support will not be included.")
        set(CHAINDB_SRCS ${CHAINDB_SRCS} chaindb/chaindb_fail_mongo.cpp)
    endif()
else()
    message("chaindb with MongoDB support not selected and will be omitted.")
    set(CHAINDB_SRCS ${CHAINDB_SRCS} chaindb/chaindb_fail_mongo.cpp)
endif()

file(GLOB HEADERS "include/eosio/chain/*.hpp"
                  "include/eosio/chain/webassembly/*.hpp"
                  "include/cyberway/chaindb/*.hpp"
                  "${CMAKE_CURRENT_BINARY_DIR}/include/eosio/chain/core_symbol.hpp" )

## SORT .cpp by most likely to change / break compile
add_library( eosio_chain
             merkle.cpp
             name.cpp
             transaction.cpp
             block_header.cpp
             block_header_state.cpp
             block_state.cpp
             fork_database.cpp
             controller.cpp
             authorization_manager.cpp
             resource_limits.cpp
             block_log.cpp
             transaction_context.cpp
             eosio_contract.cpp
             eosio_contract_abi.cpp
             chain_config.cpp
             chain_id_type.cpp
             genesis_state.cpp
             ${CMAKE_CURRENT_BINARY_DIR}/genesis_state_root_key.cpp

#             chain_config.cpp
#             block_trace.cpp
              wast_to_wasm.cpp
              wasm_interface.cpp
              wasm_eosio_validation.cpp
              wasm_eosio_injection.cpp
              apply_context.cpp
              abi_serializer.cpp
              asset.cpp

             webassembly/wavm.cpp
             webassembly/binaryen.cpp
             webassembly/wabt.cpp

             ${CHAINDB_SRCS}

#             get_config.cpp
#             global_property_object.cpp
#
#             contracts/chain_initializer.cpp


#             transaction_metadata.cpp
             ${HEADERS}
             )

target_link_libraries( eosio_chain eos_utilities fc chainbase Logging IR WAST WASM Runtime
      wasm asmjs passes cfg ast emscripten-optimizer support softfloat builtins wabt
      ${CHAINDB_LIBS})
target_include_directories( eosio_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../wasm-jit/Include"
                                   "${CMAKE_CURRENT_SOURCE_DIR}/../../externals/binaryen/src"
                                   "${CMAKE_SOURCE_DIR}/libraries/wabt"
                                   "${CMAKE_BINARY_DIR}/libraries/wabt"
                            PRIVATE ${CHAINDB_INCS}
                            )
target_compile_definitions( eosio_chain PRIVATE ${CHAINDB_DEFS})

install( TARGETS eosio_chain
   RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
)
install( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/eosio/chain/
      DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/chain
      FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "webassembly" EXCLUDE
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/eosio/chain/core_symbol.hpp DESTINATION ${CMAKE_INSTALL_FULL_INCLUDEDIR}/eosio/chain)
#if(MSVC)
#  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_log.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)
