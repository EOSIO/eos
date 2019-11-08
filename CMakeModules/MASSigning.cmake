macro(mas_sign target)

#example values:
# MAS_CERT_FINGERPRINT=C5139C2C4D7FA071EFBFD86CE44B652631C9376A
# MAS_BASE_APPID=5A4683969Z.com.example.         <<note the trailing period on this
# MAS_PROVISIONING_PROFILE="/Users/spoon/Library/MobileDevice/Provisioning Profiles/b1d57761-e5b8-4e58-b412-f1cd0f1924a1.provisionprofile"
# MAS_KEYCHAIN_GROUP=5A4683969Z.com.example.keyz
#Given the above, the executable will be signed via the certificate in the keystore matching the fingerprint and bundled with the
# specified provisioning profile. The appid will the base plus the name of the target, 5A4683969Z.com.example.keosd for example. And
# the entitlements file will have a keychain sharing group of 5A4683969Z.com.example.keyz

if(APPLE AND MAS_CERT_FINGERPRINT AND MAS_BASE_APPID AND MAS_PROVISIONING_PROFILE AND MAS_KEYCHAIN_GROUP)

  add_custom_command(TARGET ${target} POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/tools/mas_sign.sh ${MAS_CERT_FINGERPRINT} ${MAS_BASE_APPID}$<TARGET_FILE_NAME:${target}> ${MAS_PROVISIONING_PROFILE} ${MAS_KEYCHAIN_GROUP} $<TARGET_FILE_NAME:${target}>
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     VERBATIM
                     )

endif()

endmacro(mas_sign)