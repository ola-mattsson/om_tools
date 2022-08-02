
add_library(utilities)
add_library(OU::utilities ALIAS utilities)

target_include_directories(utilities
        INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
        )
target_sources(utilities
        PRIVATE
        connection_pool.cpp
        )
target_compile_features(utilities PRIVATE cxx_std_17)
