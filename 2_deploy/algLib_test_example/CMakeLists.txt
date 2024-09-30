cmake_minimum_required(VERSION 3.16)
project(algLib_test_example)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN;./lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic -Wl,-O1")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

add_definitions("-D_REENTRANT -fPIC -DLinux")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message(STATUS "ENABLE ADDRESS SANTITIZE")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../algLib/Include"
"${CMAKE_CURRENT_SOURCE_DIR}/utils/jpeg_turbo/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../algLib/3rdparty/jsoncpp/include")

LINK_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/utils/jpeg_turbo/Linux/armhf"
"${CMAKE_CURRENT_SOURCE_DIR}/../algLib/3rdparty/jsoncpp/lib")

AUX_SOURCE_DIRECTORY(./utils UTILS_LIST)
AUX_SOURCE_DIRECTORY(./ SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST} ${UTILS_LIST})

set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../algLib/3rdparty/opencv/share/OpenCV)
message(STATUS OpenCV_DIR=${OpenCV_DIR})
find_package(OpenCV REQUIRED)
message(STATUS OpenCV_LIBS=${OpenCV_LIBS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} turbojpeg alg json)

install(TARGETS ${PROJECT_NAME} DESTINATION .)
install(TARGETS ${PROJECT_NAME} DESTINATION .)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config/algLib_test_example.json DESTINATION ./config)