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

# Catch2 and pugixml are fetched via FetchContent so contributors don't need
# either installed system-wide - only network access at configure time.
include(FetchContent)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(catch2)

FetchContent_Declare(
    pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
)
FetchContent_MakeAvailable(pugixml)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")

add_executable(compiler_tests
    ${TEST_SOURCES}
    ${SOURCES}
    ${HEADERS}
)

target_include_directories(compiler_tests
    PRIVATE
        ${INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(compiler_tests
    PRIVATE
        Catch2::Catch2WithMain
        pugixml
)

# The pipeline under test always reads "ProductionRules.txt",
# "SLR_ACTION_Table.csv" and "SLR_GOTO_Table.csv" from the current working
# directory, and TreeBuilder::writeXML always writes "tree.xml" into the
# current working directory too - both are hardcoded, not parameterized.
# Rather than run tests from the repo root (which would overwrite the
# checked-in tree.xml on every run), copy the three read-only data files
# into a scratch working directory under the build tree and point the
# tests there.
set(TEST_WORKDIR "${CMAKE_BINARY_DIR}/test_workdir")
file(MAKE_DIRECTORY "${TEST_WORKDIR}")
add_custom_command(TARGET compiler_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/ProductionRules.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/SLR_ACTION_Table.csv"
        "${CMAKE_CURRENT_SOURCE_DIR}/SLR_GOTO_Table.csv"
        "${TEST_WORKDIR}/"
)

enable_testing()
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(compiler_tests WORKING_DIRECTORY "${TEST_WORKDIR}")
