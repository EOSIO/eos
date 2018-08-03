macro(eosio_additional_plugin)
    set(ADDITIONAL_PLUGINS_TARGET "${ADDITIONAL_PLUGINS_TARGET};${ARGN}" PARENT_SCOPE)
endmacro()


