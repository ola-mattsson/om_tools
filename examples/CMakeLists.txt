
add_executable(zlib_example)
add_test(zlib_example zlib_example)
target_sources(zlib_example
        PRIVATE
        zlib_example.cpp
        )
target_link_libraries(zlib_example
        PRIVATE
        OU::the_wrappers
        )

add_executable(zlib_example_same_buffer)
# doesn't work properly, don't run test until it does
#add_test(zlib_example_same_buffer zlib_example_same_buffer)
target_sources(zlib_example_same_buffer
        PRIVATE
        zlib_example_same_buffer.cpp
        )
target_link_libraries(zlib_example_same_buffer
        PRIVATE
        OU::the_wrappers
        )

add_executable(curl_example)
add_test(curl_example curl_example)
target_sources(curl_example
        PRIVATE
        curl_example.cpp
        )
target_link_libraries(curl_example
        PRIVATE
        OU::the_wrappers
        )

add_executable(curl_jsonrpc)
#add_test(curl_jsonrpc curl_jsonrpc)
target_sources(curl_jsonrpc
        PRIVATE
        curl_jsonrpc.cpp
        )
target_link_libraries(curl_jsonrpc
        PRIVATE
        OU::the_wrappers
        )

add_executable(libpq_example)
add_test(libpq_example libpq_example)
target_sources(libpq_example
        PRIVATE
        libpq_example.cpp
        )
target_link_libraries(libpq_example
        PRIVATE
        OU::the_wrappers
        )

add_executable(descriptors__example)
add_test(descriptors__example descriptors__example)
target_sources(descriptors__example
        PRIVATE
        descriptors_example.cpp
        )
target_link_libraries(descriptors__example
        PRIVATE
        OU::the_wrappers
        )
target_compile_features(descriptors__example PRIVATE cxx_std_17)

install(TARGETS libpq_example RUNTIME DESTINATION bin)
include(CPack)