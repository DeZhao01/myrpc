cmake_minimum_required(VERSION 3.16)
project(myrpc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(myrpc main.cpp tcp_server.cpp rpc_protocol.cpp)
add_executable(client client.cpp rpc_protocol.cpp)

target_link_libraries(myrpc pthread rt)
target_link_libraries(client pthread rt)