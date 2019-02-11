add_library(eosiolib_native STATIC 
eosiolib.cpp
action.cpp
chain.cpp
crypto.cpp
db.cpp
memory.cpp
permission.cpp
print.cpp
privileged.cpp
system.cpp
transaction.cpp
)

target_link_libraries( eosiolib_native PUBLIC softfloat)

target_include_directories(eosiolib_native PRIVATE ${Boost_INCLUDE_DIR}
#   PRIVATE ${CMAKE_SOURCE_DIR}/contracts/libc++/upstream/include
#   PRIVATE ${CMAKE_SOURCE_DIR}/contracts/libc++/include/libc++/upstream/include
#   PRIVATE ${CMAKE_SOURCE_DIR}/contracts/libc++/musl/include/musl/upstream/include
#    PRIVATE ${CMAKE_SOURCE_DIR}/contracts/musl/upstream/include
    PRIVATE ${CMAKE_SOURCE_DIR}/externals/magic_get/include
    PRIVATE ${CMAKE_SOURCE_DIR}/contracts
    PUBLIC ${CMAKE_SOURCE_DIR}/libraries/chain/include
    PUBLIC ${CMAKE_SOURCE_DIR}/libraries/fc/include
    PUBLIC ${CMAKE_SOURCE_DIR}/libraries/softfloat/source/include
    )
