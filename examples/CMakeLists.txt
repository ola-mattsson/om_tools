
function(make_example)
    set(options "")
    set(one_value NAME)
    set(multi_value SOURCE INCLUDE_PATHS COMPILER_FEATURES)
    cmake_parse_arguments(PAR "${option}" "${one_value}" "${multi_value}" ${ARGN})

    add_executable(${PAR_NAME})
    add_test(${PAR_NAME} "${PAR_NAME}")
    target_sources(${PAR_NAME}
        PRIVATE
        "${PAR_SOURCE}"
    )
    target_link_libraries(${PAR_NAME}
        PRIVATE
        OU::the_wrappers
        OU::utilities
    )
    target_include_directories(${PAR_NAME}
        PRIVATE
        ${PAR_INCLUDE_PATHS})
    target_compile_options(${PAR_NAME}
        PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
    )
endfunction()

make_example(NAME zlib_example SOURCE zlib_example.cpp INCLUDE_PATHS ${CMAKE_BINARY_DIR})

target_include_directories(zlib_example PRIVATE ${CMAKE_BINARY_DIR})
make_example(NAME zlib_example_same_buffer SOURCE zlib_example_same_buffer.cpp)
make_example(NAME curl_example SOURCE curl_example.cpp)
#make_example(NAME curl_jsonrpc curl_jsonrpc.cpp) # requires a server
make_example(NAME descriptors_example SOURCE descriptors_example.cpp)
make_example(NAME utilities_example SOURCE utilities.cpp)
make_example(NAME redis_example SOURCE redis_example.cpp)
make_example(NAME cache_db SOURCE cache_db.cpp)
#target_compile_features(cache_db PRIVATE cxx_std_98)

make_example(NAME libpq_example SOURCE libpq_example.cpp)
install(TARGETS libpq_example RUNTIME DESTINATION bin)

include(CPack)