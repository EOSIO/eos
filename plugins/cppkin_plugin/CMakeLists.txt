if(ENABLE_CPPKIN)

  file(GLOB HEADERS "include/eosio/cppkin_plugin/*.hpp")
  add_library( cppkin_plugin
               cppkin_plugin.cpp
               ${HEADERS} )

  target_link_libraries( cppkin_plugin chain_plugin appbase fc )
  target_include_directories( cppkin_plugin PUBLIC include )

else()
  message("ENABLE_CPPKIN=False cppkin_plugin will be omitted.")
endif()
