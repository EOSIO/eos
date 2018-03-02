
add_library( eos_egenesis_none
   egenesis_none.cpp
   include/eosio/egenesis/egenesis.hpp
   )

target_link_libraries( eos_egenesis_none eosio_chain fc )
target_include_directories( eos_egenesis_none
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_executable( embed_genesis
   embed_genesis.cpp
   )

target_link_libraries( embed_genesis eosio_chain eos_egenesis_none fc )

set( embed_genesis_args
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
   -t "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl---${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   )

MESSAGE( STATUS "egenesis: " ${EOS_EGENESIS_JSON} )

if( EOS_EGENESIS_JSON )
   list( APPEND embed_genesis_args --genesis-json "${EOS_EGENESIS_JSON}" )
endif( EOS_EGENESIS_JSON )

MESSAGE( STATUS "embed_genesis_args: " ${embed_genesis_args} )

add_custom_command(
   OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
   COMMAND embed_genesis ${embed_genesis_args}
   DEPENDS
      "${EOS_EGENESIS_JSON}"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_brief.cpp.tmpl"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_full.cpp.tmpl"
      embed_genesis )

add_library( eos_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp" include/eosio/egenesis/egenesis.hpp )
add_library( eos_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp" include/eosio/egenesis/egenesis.hpp )

target_link_libraries( eos_egenesis_brief eosio_chain fc )
target_link_libraries( eos_egenesis_full eosio_chain fc )

target_include_directories( eos_egenesis_brief
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( eos_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
