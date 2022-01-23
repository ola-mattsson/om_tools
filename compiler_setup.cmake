add_library(compiler_flags INTERFACE)
add_library(OU::compiler_flags ALIAS compiler_flags)
target_compile_features(compiler_flags INTERFACE cxx_std_98)

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
# can't use both address and thread sanitizer at the same time
option(USE_THREAD_SANITIZER "Use thread sanitiser instead of address, cannot coexist" OFF)

set(SANITIZE       $<IF:$<BOOL:${USE_SANITIZERS}>,$<IF:$<BOOL:${USE_THREAD_SANITIZER}>,-fsanitize=thread,-fsanitize=address>,>)
set(SANITIZE_FLAGS $<IF:$<BOOL:${USE_SANITIZERS}>,-fno-omit-frame-pointer,>)

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


# override target_link_libraries, makes sure all links to compiler_flags.
function(target_link_libraries _name_)
#        message(STATUS "${_name_} links to: ${ARGN}, adding compiler flags")
        _target_link_libraries(${_name_} ${ARGN} OU::compiler_flags)
endfunction()
