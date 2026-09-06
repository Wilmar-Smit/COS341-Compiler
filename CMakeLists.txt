cmake_minimum_required(VERSION 3.20)
project(compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compilation database for Zed / clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Define executable
add_executable(compiler)

# Recursively locate implementation source files and headers
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h" "src/*.hpp")

# Attach all source files and headers
target_sources(compiler
    PRIVATE
        src/main.cpp
        ${SOURCES}
        ${HEADERS}
)

# Dynamically gather all subdirectories under src/ for header includes
set(INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src")
foreach(HEADER_FILE ${HEADERS})
  get_filename_component(DIR_PATH ${HEADER_FILE} DIRECTORY)
  list(APPEND INCLUDE_DIRS ${DIR_PATH})
endforeach()
list(REMOVE_DUPLICATES INCLUDE_DIRS)

target_include_directories(compiler
    PRIVATE
        ${INCLUDE_DIRS}
)
