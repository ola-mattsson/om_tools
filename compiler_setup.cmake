add_library(compiler_flags INTERFACE)
add_library(OU::compiler_flags ALIAS compiler_flags)
target_compile_features(compiler_flags INTERFACE cxx_std_17)

# target_compile_features doesn't seem to respond to cxx_std_98 on Linux, instead you
# get the default version as if no standard was requested..
# to build with C++98 then need the old fashion way. uncomment to use C++98
# set(CMAKE_CXX_STANDARD 98)

set(CLANG_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        )
set(GCC_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        )

option(USE_SANITIZERS "Should we find all the hard to find bugs?" ON)
if (USE_SANITIZERS)
    set(SANITIZE "-fsanitize=address")
    set(SANITIZE_FLAGS "-fno-omit-frame-pointer")
endif ()

set(clang "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang>")
set(gcc "$<COMPILE_LANG_AND_ID:CXX,GNU>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(compiler_flags INTERFACE
        "$<${clang}:$<BUILD_INTERFACE:${CLANG_WARNING_FLAGS};${SANITIZE};${SANITIZE_FLAGS}>>"
        "$<${gcc}:$<BUILD_INTERFACE:${GCC_WARNING_FLAGS};${SANITIZE};${SANITIZE_FLAGS}>>"
        "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
        )
target_link_options(compiler_flags
        INTERFACE
        ${SANITIZE}
        )

find_package(Threads)
target_link_libraries(compiler_flags
        INTERFACE
        Threads::Threads
        )
target_compile_definitions(compiler_flags
        INTERFACE
        )