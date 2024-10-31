cmake_minimum_required(VERSION 3.27)
project(Proxy)

set(CMAKE_CXX_STANDARD 14)

# Set debug build type to add debug symbols
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Ensure OpenSSL is found and required
find_package(OpenSSL REQUIRED)

add_executable(proxy
        Proxy/main.cpp
        Proxy/Logger.h
        Proxy/LoadBalancer.h
)

target_include_directories(proxy PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(proxy OpenSSL::SSL OpenSSL::Crypto)
