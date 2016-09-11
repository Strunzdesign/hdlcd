cmake_minimum_required(VERSION 2.6)

project(hdlcd)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER} has no C++11 support.")
endif()

# Add custom cxx flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")

# Automatically set the version number
set(HDLCD_VERSION_MAJOR  \"1\")
set(HDLCD_VERSION_MINOR  \"3pre\")
include_directories("${PROJECT_BINARY_DIR}/src")
configure_file (
    "${PROJECT_SOURCE_DIR}/src/Config.h.in"
    "${PROJECT_BINARY_DIR}/src/Config.h"
)

# Add include path for Microsoft Windows
set(HDLCD_DEVEL_INCLUDE_DIR "/usr/local/include" CACHE FILEPATH "/usr/local/include")
if (NOT EXISTS ${HDLCD_DEVEL_INCLUDE_DIR}/HdlcdConfig.h)
    message(SEND_ERROR "Can't find the HDLC daemon header files in ${HDLCD_DEVEL_INCLUDE_DIR}")
endif()
include_directories(${HDLCD_DEVEL_INCLUDE_DIR})

add_subdirectory(src)
