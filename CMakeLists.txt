cmake_minimum_required(VERSION 3.20)
project(om_tools)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

include(compiler_setup.cmake)

if (RUN_CONAN AND NOT CONAN_WAS_RUN)
    execute_process(COMMAND conan remove --locks)

    include(${CMAKE_SOURCE_DIR}/conan.cmake)
    conan_cmake_install(
            #            OUTPUT_QUIET
            PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR}
            BUILD missing
            REMOTE conancenter #<your remote>
    )
    set(CONAN_WAS_RUN TRUE CACHE BOOL "" )
endif ()

enable_testing()

add_subdirectory(utilities)
add_subdirectory(wrappers)
add_subdirectory(examples)
add_subdirectory(unittests)




