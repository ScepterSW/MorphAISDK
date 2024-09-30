cmake_minimum_required(VERSION 3.16)
PROJECT(Alg)

if(${CMAKE_BUILD_TYPE} MATCHES "Release")
    message(STATUS "Release")
else()
    message(STATUS "Debug")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/algLib)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/algLib_test_example)