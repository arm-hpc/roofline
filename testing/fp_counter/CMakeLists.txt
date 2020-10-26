## Andrea Brunato andrea.brunato@arm.com
## Make it easily defined by the user

cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_FLAGS "-Werror=implicit-function-declaration")
set (CMAKE_CXX_STANDARD 11)
file(GLOB SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "*.cpp")
add_library(fp_counter SHARED main.cpp ${SOURCES})

if(NOT DEFINED ENV{DYNAMORIO_BUILD_DIR})
	message(FATAL_ERROR "Please define the following environment variable: export DYNAMORIO_BUILD_DIR=<path/to/dr/build/folder> ")
endif ()

string(CONCAT DR_PACKAGE $ENV{DYNAMORIO_BUILD_DIR} "/cmake/")
set(DynamoRIO_DIR ${DR_PACKAGE} CACHE PATH
	"DynamoRIO installation's cmake directory")

find_package(DynamoRIO)
if (NOT DynamoRIO_FOUND)
 message(FATAL_ERROR "DynamoRIO package required to build")
endif(NOT DynamoRIO_FOUND)

string(CONCAT DR_INCLUDE $ENV{DYNAMORIO_BUILD_DIR} "/include/")
string(CONCAT DR_EXT_INCLUDE $ENV{DYNAMORIO_BUILD_DIR} "/ext/include/")
include_directories(${DR_INCLUDE})
include_directories("./include/")
include_directories(${DR_EXT_INCLUDE})

## TODO: You probably won't require __ALL__ of these extensions. Eventually remove them
configure_DynamoRIO_client(fp_counter)
use_DynamoRIO_extension(fp_counter drmgr)
