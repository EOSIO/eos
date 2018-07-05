file(GLOB HEADERS "include/eosio/wallet_plugin/*.hpp")

if(APPLE)
  set(SE_WALLET_SOURCES se_wallet.cpp macos_user_auth.m)
  set_source_files_properties(macos_user_presence.m PROPERTIES COMPILE_FLAGS "-x objective-c")

  find_library(security_framework security)
  find_library(localauthentication_framework localauthentication)
  find_library(corefoundation_framework corefoundation)
  find_library(cocoa_framework cocoa)

  if(MAS_KEYCHAIN_GROUP)
    add_definitions(-DMAS_KEYCHAIN_GROUP=${MAS_KEYCHAIN_GROUP})
  endif(MAS_KEYCHAIN_GROUP)
endif(APPLE)

add_library( wallet_plugin
             wallet.cpp
             wallet_plugin.cpp
             wallet_manager.cpp
             ${SE_WALLET_SOURCES}
             ${HEADERS} )

target_link_libraries( wallet_plugin eosio_chain appbase ${security_framework} ${corefoundation_framework} ${localauthentication_framework} ${cocoa_framework})
target_include_directories( wallet_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
