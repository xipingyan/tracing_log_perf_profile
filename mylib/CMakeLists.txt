cmake_minimum_required(VERSION 2.4.0)

SET(LIB_NAME lib1)
project(lib1)

# Find source files
file(GLOB SOURCES *.cpp)

# Include header files
include_directories(./)

# Create shared library
add_library(${LIB_NAME} SHARED ${SOURCES})

