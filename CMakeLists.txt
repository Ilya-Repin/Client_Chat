cmake_minimum_required(VERSION 3.29)
project(Chat_client)

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)

include_directories(src)

add_executable(Chat_client src/main.cpp
        src/crypto/crypto.cpp
        src/crypto/crypto.h
        src/network/network.cpp
        src/network/network.h
        src/network/register.cpp
        src/network/register.h
        src/app/app.cpp
        src/app/app.h)

target_link_libraries(Chat_client PRIVATE CONAN_PKG::cryptopp CONAN_PKG::boost)