add_library(yubihsm upstream/aes_cmac/aes.c
                    upstream/aes_cmac/aes_cmac.c
                    upstream/common/hash.c
                    upstream/common/pkcs5.c
                    upstream/common/rand.c
                    upstream/lib/error.c
                    upstream/lib/lib_util.c
                    upstream/lib/yubihsm.c)

if(WIN32)
   target_sources(yubihsm upstream/lib/yubihsm_winhttp.c
                          upstream/lib/yubihsm_usb.c
                          upstream/lib/yubihsm_winusb.c)
   find_package(OpenSSL REQUIRED)
   target_link_libraries(yubihsm winusb ws2_32 winhttp setupapi OpenSSL::Crypto)
   target_compile_definitions(yubihsm PRIVATE -DVERSION="yubihsmwallet")
else()
   target_sources(yubihsm PRIVATE yubihsm_beast.cpp)

   if(NOT LIBUSB_1_INCLUDE_DIR)
      find_path(LIBUSB_1_INCLUDE_DIR NAMES libusb.h PATH_SUFFIXES libusb-1.0)
   endif()

   if(LIBUSB_1_INCLUDE_DIR)
      target_sources(yubihsm PRIVATE upstream/lib/yubihsm_usb.c yubihsm_dyn_libusb.cpp)
      target_include_directories(yubihsm PRIVATE "${LIBUSB_1_INCLUDE_DIR}")
   else()
      message(WARNING "libusb 1.0 not found, direct USB support for yubihsm disabled")
      target_sources(yubihsm PRIVATE yubihsm_nousb.c)
   endif()

   target_link_libraries(yubihsm PRIVATE fc)
endif()

target_compile_definitions(yubihsm PRIVATE -DSTATIC)
target_include_directories(yubihsm PUBLIC upstream/lib)
