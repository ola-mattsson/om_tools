
find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(Boost REQUIRED)
find_package(hiredis REQUIRED)
find_package(Libevent REQUIRED)
find_package(fmt REQUIRED)

add_library(the_wrappers INTERFACE)
add_library(OU::the_wrappers ALIAS the_wrappers)

target_link_libraries(the_wrappers
        INTERFACE
        ZLIB::ZLIB
        PostgreSQL::PostgreSQL
        CURL::libcurl
        Boost::headers
        fmt::fmt
        hiredis::hiredis
        libevent::libevent
        )
target_compile_features(the_wrappers INTERFACE cxx_std_17)

target_include_directories(the_wrappers
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )
