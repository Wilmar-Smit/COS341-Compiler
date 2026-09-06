cmake_minimum_required(VERSION 3.12)
project(compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define executable
add_executable(compiler)

# Configure build dependencies (includes main.cpp and recursive src files)
target_sources(compiler
    PRIVATE
        src/main.cpp
)

# Recursively locate source and header files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h")

target_sources(compiler
    PRIVATE
        ${SOURCES}
        ${HEADERS}
)

# Expose header search paths for recursive directories
target_include_directories(compiler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)