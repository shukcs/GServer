cmake_minimum_required(VERSION 2.8.9)
set (USE_32BITS 1)
include(precheck.cmake)

SET(PROJECT_BINARY_DIR ${CMAKE_BINARY_DIR}/bin/})
set(CMAKE_SHARED_LIBRARY_PREFIX ${PROJECT_BINARY_DIR})


project (GServer)

add_subdirectory(tinyxml)
add_subdirectory(protobuf)
add_subdirectory(gshare)
add_subdirectory(vgmysql)
add_subdirectory(uavandqgc)
add_subdirectory(gsocket)
add_subdirectory(zlib-1.2.11)
