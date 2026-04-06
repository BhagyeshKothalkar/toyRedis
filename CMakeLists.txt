cmake_minimum_required(VERSION 3.24)
project(Radis VERSION 0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set_source_files_properties(src/TcpServer.c src/TcpClient.c PROPERTIES LANGUAGE CXX)
add_executable(redis_server src/main.cpp src/TcpServer.cpp)
add_executable(dummy_client src/testxlient.cpp)

target_include_directories(redis_server PRIVATE include)
target_include_directories(dummy_client PRIVATE include)

if(MSVC)

    target_compile_options(redis_server PRIVATE /W4)
    target_compile_options(dummy_client PRIVATE /W4)
else()

    target_compile_options(redis_server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(dummy_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# cmake -B build -G Ninja && cmake --build build