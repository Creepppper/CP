cmake_minimum_required(VERSION 3.16)
project(mini_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mini_parser src/main.cpp src/parser.cpp)

# GCC before 9.1 (for example an old MinGW) needs a separate library for std::filesystem.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
    target_link_libraries(mini_parser stdc++fs)
endif()
