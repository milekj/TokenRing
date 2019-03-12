cmake_minimum_required(VERSION 3.13)
project(TokenRing)

set(CMAKE_CXX_STANDARD 14)

add_executable(tokenRing tcp.cpp old.cpp udp.cpp tokenRing.cpp tokenRing.h)