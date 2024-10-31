cmake_minimum_required(VERSION 3.27)
project(Proxy)

set(CMAKE_CXX_STANDARD 14)

# Set debug build type to add debug symbols
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Ensure OpenSSL is found and required
find_package(OpenSSL REQUIRED)

# Add the executable and specify the source files
add_executable(software_engineering_product_topteam
        Proxy/main.cpp
        Proxy/Logger.h
        Proxy/LoadBalancer.h
)

# Include the OpenSSL directories
target_include_directories(software_engineering_product_topteam PRIVATE ${OPENSSL_INCLUDE_DIR})

# Link the OpenSSL libraries
target_link_libraries(software_engineering_product_topteam OpenSSL::SSL OpenSSL::Crypto)
