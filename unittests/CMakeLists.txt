find_package(GTest REQUIRED)
add_executable(redis_ut)
set(CMAKE_CXX_STANDARD 20)
target_compile_features(redis_ut PRIVATE cxx_std_20)
#add_test(redis_ut redis_test)
target_sources(redis_ut
        PRIVATE
        redis_ut.cpp
        )
target_link_libraries(redis_ut
        PRIVATE
        OU::the_wrappers
        GTest::GTest
        OU::utilities
        OU::compiler_flags
        )
