# will be implictly used for any compilation unit if not overrided by SYSTEM_INCLUDE_FOLDERS parameter
# these directories go as -isystem <dir> to avoid warnings from code of third-party libraries

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apifiny.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/apifiny.bios/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apifiny.msig/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/apifiny.msig/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apifiny.system/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/apifiny.system/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apifiny.token/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/apifiny.token/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/apifiny.wrap/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/apifiny.wrap/)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/old_versions/v1.6.0-rc3/apifiny.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/old_versions/v1.6.0-rc3/apifiny.bios/)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/old_versions/v1.7.0-develop-preactivate_feature/apifiny.bios/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/old_versions/v1.7.0-develop-preactivate_feature/apifiny.bios/)
