file(GLOB HEADERS "include/eos/py_plugin/*.hpp")
set(src 
    py_plugin.cpp 
  )

add_library( py_plugin
             STATIC
             py_plugin.cpp
             hello.c
             ${HEADERS} )

target_link_libraries( py_plugin appbase chain_api_plugin producer_plugin chain_plugin net_plugin http_plugin eos_chain fc)
target_include_directories( py_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "/Users/newworld/anaconda/include/python3.6m")

install( TARGETS
   py_plugin

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/eos/py_plugin" )
