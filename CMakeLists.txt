cmake_minimum_required(VERSION 3.2)

project(myprofile)

include_directories(./)
file(GLOB SOURCES "./*.cpp")

add_executable(test_profile ${SOURCES})