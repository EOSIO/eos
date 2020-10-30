if(NOT TARGET tpm-helpers)
    return()
endif()

find_program(SWTPM_FOUND swtpm)
find_program(STARTAUTHSESSION_FOUND tpm2_startauthsession)
find_program(CREATEPRIMARY_FOUND tpm2_createprimary)
find_program(FLUSHCONTEXT_FOUND tpm2_flushcontext)

if(NOT SWTPM_FOUND OR NOT STARTAUTHSESSION_FOUND OR NOT CREATEPRIMARY_FOUND OR NOT FLUSHCONTEXT_FOUND)
   message(WARNING "Unable to find applications required for TPM unit tests, TPM tests disabled")
   return()
endif()

add_executable(tpm_test tpm_tests.cpp)
target_link_libraries(tpm_test PRIVATE tpm-helpers)

add_test(NAME tpm_test COMMAND tests/tpm_tests/tpm_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})