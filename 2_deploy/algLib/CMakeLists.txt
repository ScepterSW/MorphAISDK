cmake_minimum_required(VERSION 3.16)
project(alg)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_SKIP_INSTALL_RPATH_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-Bsymbolic -Wl,-O2")

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O2 -Wall -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

add_definitions("-D_REENTRANT -fPIC -D ALG_EXPORT_ON -D Linux -D RV1126")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    message(STATUS "ENABLE ADDRESS SANTITIZE")
    set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif ()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/"
"${CMAKE_CURRENT_SOURCE_DIR}/Include"
"${CMAKE_CURRENT_SOURCE_DIR}/Src"
"${CMAKE_CURRENT_SOURCE_DIR}/Src/Postprocess"
"${CMAKE_CURRENT_SOURCE_DIR}//3rdparty/jsoncpp/include"
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jsoncpp/include")

LINK_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/jsoncpp/lib")

message(STATUS "+++++++++++++++++++++")
message(STATUS "MODEL_TYPE: ${MODEL_TYPE}")
message(STATUS "+++++++++++++++++++++")

if (MODEL_TYPE STREQUAL "yolov5")
    add_definitions(-DYOLOV5)
elseif(MODEL_TYPE STREQUAL "yolov8")
    add_definitions(-DYOLOV8)
elseif(MODEL_TYPE STREQUAL "vzense_box")
    add_definitions(-DVZENSE_BOX)
endif()

set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/opencv/share/OpenCV)
message(STATUS OpenCV_DIR=${OpenCV_DIR})
find_package(OpenCV REQUIRED)
message(STATUS OpenCV_LIBS=${OpenCV_LIBS})

AUX_SOURCE_DIRECTORY(./Src SOURCES)
AUX_SOURCE_DIRECTORY(./Src/Postprocess POSTPROCESS)

ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCES} ${POSTPROCESS})

target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} rknn_api rga pthread dl json)

install(TARGETS ${PROJECT_NAME} DESTINATION ./lib)
