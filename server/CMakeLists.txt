cmake_minimum_required(VERSION 3.10)

project(server)

set(CMAKE_CXX_STANDARD 20)

set(OPENSSL_ROOT_DIR "C:/msys64/mingw64")
add_definitions(-D_WIN32_WINNT=0x0A00)

add_executable(server src/main.cpp)

target_include_directories(server PRIVATE include)

find_package(OpenSSL REQUIRED)

target_compile_definitions(server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)


if (WIN32)
    target_link_libraries(server PRIVATE ws2_32 crypt32 OpenSSL::SSL OpenSSL::Crypto)
endif()
