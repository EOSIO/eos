file(GLOB HEADERS "include/eos/debug_producer/*.hpp")

add_library( eos_debug_producer
             debug_api.cpp
             debug_producer.cpp
           )

target_link_libraries( eos_debug_producer eos_chain eos_app )
target_include_directories( eos_debug_producer
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   eos_debug_producer

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
