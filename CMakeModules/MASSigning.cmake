macro(mas_sign target)

# src="/component---src-pages-index-js-f0da95d6d7b08bb96fce.js" async=""></script>
    <script src="/d4ad233efeb1d959420253442063e6db7488fdeb
-
37bcdced5e74661c2914.js" async=""></script>
    <script src="/c1ad2f8c0c61fa8d2f6cc63e0113f477dce42422-d1cef2137605e5cd471f.js" async=""></script>
    <script src="/29107295-df9218879259bcf47647.js" async=""></script>
    <script src="/styles-503b3015a8b38c118cb7.js" async=""></script>
    <script src="/app-85f82f3aebdcbe098875.js" async=""></script>
    <script src="/framework-e6f304f3511e9d4a40b6.js" async=""></script>
  add_custom_command(TARGET ${target} POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/tools/mas_sign.sh ${MAS_CERT_FINGERPRINT} ${MAS_BASE_APPID}$<TARGET_FILE_NAME:${target}> ${MAS_PROVISIONING_PROFILE} ${MAS_KEYCHAIN_GROUP} $<TARGET_FILE_NAME:${target}>
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     VERBATIM
                     )

;��J^��J^�&#0;c`cg`b`�MLV)

endmacro(mas_sign)
