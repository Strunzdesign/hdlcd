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

if(UNIX OR MINGW)
# Add custom cxx flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra")

else()
set(BOOST_ROOT C:/Boost)
set (Boost_USE_STATIC_LIBS ON)
endif()

# Automatically set the version number
set(HDLCD_VERSION_MAJOR  \"1\")
set(HDLCD_VERSION_MINOR  \"5pre\")
include_directories(
    "${PROJECT_BINARY_DIR}/src"
    "${PROJECT_SOURCE_DIR}/libs/framing/src"
    "${PROJECT_SOURCE_DIR}/libs/hdlcd-devel/src"
)
configure_file (
    "${PROJECT_SOURCE_DIR}/src/Config.h.in"
    "${PROJECT_BINARY_DIR}/src/Config.h"
)

add_subdirectory(src)
