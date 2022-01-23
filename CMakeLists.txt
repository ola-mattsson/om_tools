cmake_minimum_required(VERSION 3.21)
project(om_tools)
message(STATUS "CMAKE_MODULE_PATH ${}CMAKE_MODULE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})
message(STATUS "CMAKE_MODULE_PATH ${}CMAKE_MODULE_PATH}")

include(compiler_setup.cmake)


add_subdirectory(wrappers)
add_subdirectory(examples)
