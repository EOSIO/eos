add_library(amqp
        reliable_amqp_publisher.cpp
        retrying_amqp_connection.cpp
        util.cpp
        )
target_include_directories(amqp PUBLIC include)
target_link_libraries(amqp fc amqpcpp)
