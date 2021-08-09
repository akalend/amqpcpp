cmake_minimum_required(VERSION 2.8.12)

project(amqpcpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_SSL_SUPPORT "Enable SSL support" ON)

add_library(
    amqpcpp
    SHARED
    src/AMQP.cpp
    src/AMQPBase.cpp
    src/AMQPException.cpp
    src/AMQPMessage.cpp
    src/AMQPExchange.cpp
    src/AMQPQueue.cpp
    include/AMQPcpp.h
)
target_include_directories(amqpcpp PUBLIC include/)

add_library(
    amqpcpp-static
    STATIC
    src/AMQP.cpp
    src/AMQPBase.cpp
    src/AMQPException.cpp
    src/AMQPMessage.cpp
    src/AMQPExchange.cpp
    src/AMQPQueue.cpp
    include/AMQPcpp.h
)
target_include_directories(amqpcpp-static PUBLIC include/)

if (NOT ENABLE_SSL_SUPPORT)
    add_definitions(-DAMQP_NO_SSL)
endif()

if(MSVC)
    target_compile_options(amqpcpp PUBLIC "/Zc:__cplusplus")
    target_compile_options(amqpcpp-static PUBLIC "/Zc:__cplusplus")
endif()

add_subdirectory(rabbitmq-c)
target_include_directories(rabbitmq PUBLIC rabbitmq-c/librabbitmq/)

if (ENABLE_SSL_SUPPORT)
    target_link_libraries(amqpcpp rabbitmq ssl crypto)
    target_link_libraries(amqpcpp-static rabbitmq ssl crypto)
else()
    target_link_libraries(amqpcpp rabbitmq)
    target_link_libraries(amqpcpp-static rabbitmq)
endif()
