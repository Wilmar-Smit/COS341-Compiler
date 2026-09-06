cmake_minimum_required(VERSION 3.28)
project(COS341-Compiler CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm")

add_executable(compiler src/main.cpp)

target_sources(compiler
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
            ${MODULE_SOURCES}
)
