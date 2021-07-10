add_library(eos-vm INTERFACE IMPORTED)

set_target_properties(eos-vm PROPERTIES
                      INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../../include"
                      INTERFACE_COMPILE_OPTIONS "-Wno-register"
                      INTERFACE_COMPILE_DEFINITIONS "EOS_VM_SOFTFLOAT"
                      INTERFACE_LINK_LIBRARIES "softfloat"
                      INTERFACE_COMPILE_FEATURES cxx_std_17
                      )

