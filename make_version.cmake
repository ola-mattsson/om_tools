
if (NOT GIT_HASH_PATH)
        message(STATUS "Usage: cmake -DGIT_HASH_PATH=<path> make_version.cmake")
        return()
endif()

execute_process(
        COMMAND git log -1 --format=%h
        OUTPUT_VARIABLE HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

if (EXISTS ${GIT_HASH_PATH}/git_hash.h)
        file(READ ${GIT_HASH_PATH}/git_hash.h CONTENTS)
        # try to find the hash in the file contents
        string(FIND "${CONTENTS}" "${HASH}" SAME_HASH)
else()
        set(CONTENTS "")
endif()

if (NOT SAME_HASH EQUAL -1)
        message(STATUS "Same commit hash, don't update")
else()
        message(STATUS "New commit hash, force rebuild")
        file(WRITE ${GIT_HASH_PATH}/git_hash.h
                "#define git_commit_hash \"${HASH}\"")
endif()
