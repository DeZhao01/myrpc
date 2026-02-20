cmake_minimum_required(VERSION 3.16)
project(CLionProject)

set(CMAKE_CXX_STANDARD 20)

add_executable(CLionProject main.cpp
        rpc_protocol.h
        tcp_server.h
        tcp_server.cpp)
