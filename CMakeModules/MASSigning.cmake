if(APPLE AND MAS_CERT_FINGERPRINT AND MAS_BASE_APPID AND MAS_PROVISIONING_PROFILE)
  set(MAS_ENABLED ON)
  if(MAS_KEYCHAIN_GROUP)
    set(MAS_LEGACY ON)
  endif()
endif()

macro(mas_sign target) #optional argument that forces keygroup when MAS_KEYCHAIN_GROUP not set

#example values:
# MAS_CERT_FINGERPRINT=C5139C2C4D7FA071EFBFD86CE44B652631C9376A
# MAS_BASE_APPID=5A4683969Z.com.example.         <<note the trailing period on this
# MAS_PROVISIONING_PROFILE="/Users/spoon/Library/MobileDevice/Provisioning Profiles/b1d57761-e5b8-4e58-b412-f1cd0f1924a1.provisionprofile"
#Given the above, the executable will be signed via the certificate in the keystore matching the fingerprint and bundled with the
# specified provisioning profile. The appid will the base plus the name of the target, 5A4683969Z.com.example.keosd for example.
#
#Additionally, it is possible to specify a keychain group to place the keys in. By default this will be 5A4683969Z.com.example.keosd in
# the above example. But, it is possible to manually specify a different key group. Specifying a keychain group was required before 3.0 and
# users who have keys created in the SE before 3.0 will need to continue using the manually specified keychain group to have access to those
# keys. To specify a keychain group manually define MAS_KEYCHAIN_GROUP, for example:
# MAS_KEYCHAIN_GROUP=5A4683969Z.com.example.keyz

if(MAS_ENABLED)
  if(MAS_KEYCHAIN_GROUP)
    set(COMPUTED_KEYCHAIN ${MAS_KEYCHAIN_GROUP})
  elseif(${ARGC} EQUAL 2)
    set(COMPUTED_KEYCHAIN ${MAS_BASE_APPID}${ARGV1})
  else()
    set(COMPUTED_KEYCHAIN ${MAS_BASE_APPID}$<TARGET_FILE_NAME:${target}>)
  endif()

  add_custom_command(TARGET ${target} POST_BUILD
                     COMMAND ${CMAKE_SOURCE_DIR}/tools/mas_sign.sh ${MAS_CERT_FINGERPRINT} ${MAS_BASE_APPID}$<TARGET_FILE_NAME:${target}> ${MAS_PROVISIONING_PROFILE} ${COMPUTED_KEYCHAIN} $<TARGET_FILE_NAME:${target}>
                     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                     VERBATIM
                     )

endif()

endmacro(mas_sign)