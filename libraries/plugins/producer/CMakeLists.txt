file(GLOB HEADERS "include/eos/producer/*.hpp")

add_library( eos_producer 
             producer.cpp
           )

target_link_libraries( eos_producer appbase eos_chain eos_utilities )
target_include_directories( eos_producer
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   eos_producer

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
