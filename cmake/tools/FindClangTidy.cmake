find_program(CLANG_TIDY_COMMAND NAMES clang-tidy)

if(NOT CLANG_TIDY_COMMAND)

    message(WARNING "CMake_RUN_CLANG_TIDY is ON but clang-tidy is not found!")
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE)

else()
    
    message(STATUS "CMake_RUN_CLANG_TIDY is ON")

    # Seems that clang-tidy fails to recognize the /EHsc after -- (the parameters given to cl.exe).
    # Refer to https://gitlab.kitware.com/cmake/cmake/-/issues/20512 about the issue.

    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}" "--extra-arg=/EHsc" CACHE STRING "" FORCE)

endif()
