cmake_minimum_required(VERSION 3.0)
include(FetchContent)

set(LIB_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tokenize)
set(TEST_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

option(BUILD_TOKENIZE_TESTS "Builds tests" OFF)

project(tokenize)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Debug build type configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g)
    add_compile_definitions(_DEBUG)
endif()

# Release build type configuration
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

add_library(tokenize INTERFACE)
add_library(tokenize::tokenize ALIAS tokenize)

target_include_directories(tokenize INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (BUILD_TOKENIZE_TESTS)
    FetchContent_Declare(Catch2 
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v2.x
    )
    FetchContent_MakeAvailable(Catch2)
    add_executable(tests ${TEST_SOURCE_DIRECTORY}/main.cpp)
    target_link_libraries(tests Catch2)
    target_link_libraries(tests tokenize)
endif()
