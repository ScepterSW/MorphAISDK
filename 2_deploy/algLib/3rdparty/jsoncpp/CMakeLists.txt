cmake_minimum_required(VERSION 3.16)

set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic -Wl,-O1")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions("-fPIC")

project(json)

include_directories(./include)

set(JSONCPP_SOURCES
    ./src/json_tool.h
    ./src/json_reader.cpp
    ./src/json_valueiterator.inl
    ./src/json_value.cpp
    ./src/json_writer.cpp
)

add_library(${PROJECT_NAME} STATIC ${PUBLIC_HEADERS} ${JSONCPP_SOURCES})