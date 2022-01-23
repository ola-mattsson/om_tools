

add_executable(zlib_example)
target_sources(zlib_example
        PRIVATE
        zlib_compression.cpp
        )
target_link_libraries(zlib_example PRIVATE
        OU::the_wrappers
        )

add_executable(curl_example)