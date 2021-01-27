find_path(TSS2_INCLUDE_DIR NAMES tss2_esys.h PATH_SUFFIXES tss2)
if(NOT TSS2_INCLUDE_DIR)
    message(STATUS "tpm2-tss headers not found, disabling TPM signing support")
    return()
endif()

set(PREVIOUS_CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
if(TPM2TSS_STATIC)
   set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
else()
   set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()

find_library(TSS2_LDR_LIBRARY NAMES tss2-tctildr)
find_library(TSS2_ESYS_LIBRARY NAMES tss2-esys)
find_library(TSS2_RC_LIBRARY NAMES tss2-rc)
find_library(TSS2_MU_LIBRARY NAMES tss2-mu)
if(TPM2TSS_STATIC)
   find_library(TSS2_SYS_LIBRARY NAMES tss2-sys)
   find_library(TSS2_TCTI_DEVICE_LIBRARY NAMES tss2-tcti-device)
   find_library(TSS2_TCTI_MSSIM_LIBRARY NAMES tss2-tcti-mssim)
   find_library(TSS2_TCTI_SWTPM_LIBRARY NAMES tss2-tcti-swtpm)
endif()

set(CMAKE_FIND_LIBRARY_SUFFIXES "${PREVIOUS_CMAKE_FIND_LIBRARY_SUFFIXES}")
unset(PREVIOUS_CMAKE_FIND_LIBRARY_SUFFIXES)

if(NOT TSS2_LDR_LIBRARY OR NOT TSS2_ESYS_LIBRARY OR NOT TSS2_RC_LIBRARY OR NOT TSS2_MU_LIBRARY)
    message(STATUS "tpm2-tss library not found, disabling TPM support")
    return()
endif()

add_library(tpm-helpers tpm-helpers.cpp)
target_include_directories(tpm-helpers PUBLIC include PRIVATE "${TSS2_INCLUDE_DIR}")
target_link_libraries(tpm-helpers PUBLIC fc "${TSS2_LDR_LIBRARY}" "${TSS2_ESYS_LIBRARY}" "${TSS2_RC_LIBRARY}" "${TSS2_MU_LIBRARY}"
                                            "${TSS2_SYS_LIBRARY}" "${TSS2_TCTI_DEVICE_LIBRARY}" "${TSS2_TCTI_MSSIM_LIBRARY}"
                                            "${TSS2_TCTI_SWTPM_LIBRARY}")
