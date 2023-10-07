cmake_minimum_required(VERSION 3.2)

set(PRJ_NAME myprofile)
project(${PRJ_NAME})

include_directories(./)
file(GLOB SOURCES "./*.cpp")

add_executable(${PRJ_NAME} ${SOURCES})

if (UNIX)
target_link_libraries(${PRJ_NAME} 
    dl # For dladdr
)
endif (UNIX)

