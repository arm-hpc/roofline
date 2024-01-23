## Andrea Brunato andrea.brunato@arm.com
## Make it easily defined by the user

project(roofline)
cmake_minimum_required(VERSION 2.8)

set (CMAKE_CXX_FLAGS "-Wall -Werror=implicit-function-declaration")
set (CMAKE_CXX_STANDARD 11)
file(GLOB SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "*.cpp")
add_library(roofline SHARED main.cpp ${SOURCES})

# The target architecture.
# For cross-compilation this should still work as you're supposed to set this var.
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set(ARM 1) # This means AArch32.
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "^aarch64")
  set(AARCH64 1)
else ()
  set(X86 1) # This means IA-32 or AMD64
endif ()

if (X86)
  set(ARCH_NAME x86)
elseif (ARM)
  set(ARCH_NAME arm)
elseif (AARCH64)
  set(ARCH_NAME aarch64)
else ()
  message(FATAL_ERROR "Unknown architecture target")
endif ()

# Pass the target architecture
## TODO: I guess this could be done in a more elegant way.
if(ARM OR AARCH64)
	add_definitions(-DFLOATING_POINTS_ARM)
else()
	add_definitions(-DFLOATING_POINTS_X86)
endif()


option(WITH_VALIDATION "Provides information about client internals for better debugging" OFF)
if(WITH_VALIDATION)
	add_definitions(-DVALIDATE)
endif()

option(WITH_VALIDATION_VERBOSE "Provide more verbose information about client internals" OFF)
if(WITH_VALIDATION_VERBOSE)
	add_definitions(-DVALIDATE)
	add_definitions(-DVALIDATE_VERBOSE)
endif()


string(CONCAT DR_PACKAGE ${CMAKE_CURRENT_SOURCE_DIR} "/../dynamorio/build/cmake/")
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
configure_DynamoRIO_client(roofline)
use_DynamoRIO_extension(roofline drmgr)
use_DynamoRIO_extension(roofline drwrap)
use_DynamoRIO_extension(roofline drutil)
use_DynamoRIO_extension(roofline drreg)
use_DynamoRIO_extension(roofline drx)
use_DynamoRIO_extension(roofline drsyms)
use_DynamoRIO_extension(roofline droption)
