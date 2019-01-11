file(GLOB HEADERS "*.hpp")
add_library(kafka_plugin
        kafka_plugin.cpp kafka.cpp try_handle.cpp
        ${HEADERS})

find_package(Cppkafka)
find_package(RdKafka)

target_include_directories(kafka_plugin PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" ${CPPKAFKA_INCLUDE_DIR})
target_link_libraries(kafka_plugin chain_plugin appbase ${CPPKAFKA_LIBRARY} RdKafka::rdkafka)
