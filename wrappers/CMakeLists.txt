
add_library(the_wrappers INTERFACE)
add_library(OU::the_wrappers ALIAS the_wrappers)

target_link_libraries(the_wrappers
        INTERFACE
        ZLIB::ZLIB
        )
target_include_directories(the_wrappers
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )
