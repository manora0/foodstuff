cmake_minimum_required(VERSION 3.10)

project(server)

set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_WIN32_WINNT=0x0A00)

add_library(sqlite3 STATIC include/sqlite3.c)
target_include_directories(server PUBLIC include)

add_executable(server src/main.cpp src/routes/user.cpp src/routes/auth.cpp src/routes/recipes.cpp)

target_include_directories(server PRIVATE include)

find_package(OpenSSL REQUIRED)

target_compile_definitions(server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# find_library(SODIUM_LIB sodium REQUIRED)

if(WIN32)
  target_link_libraries(server PRIVATE ws2_32 crypt32 OpenSSL::SSL OpenSSL::Crypto sqlite3 ${SODIUM_LIB})
else()
  target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto sqlite3 ${SODIUM_LIB})
endif()