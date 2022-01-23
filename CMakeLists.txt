cmake_minimum_required(VERSION 3.21)
project(appola_toolsbox)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

find_package(Boost REQUIRED)
find_package(ZLIB REQUIRED)

add_subdirectory(wrappers)
add_subdirectory(examples)
