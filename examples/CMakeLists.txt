

add_executable(zlib_example)
target_sources(zlib_example
        PRIVATE
        zlib_compression.cpp
        )
target_link_libraries(zlib_example
        PRIVATE
        OU::the_wrappers
        )

add_executable(curl_example)
target_sources(curl_example
        PRIVATE
        curl_example.cpp
        )
target_link_libraries(curl_example
        PRIVATE
        Boost::boost
        CURL::libcurl
        OU::the_wrappers
        )

add_executable(libpq_example)
target_sources(libpq_example
        PRIVATE
        libpq_example.cpp
        )
target_link_libraries(libpq_example
        PRIVATE
        OU::the_wrappers
        PostgreSQL::PostgreSQL
        Boost::boost
        )