# FindClangTidy.cmake — Cross-platform clang-tidy integration
#
# This module finds clang-tidy and configures CMAKE_CXX_CLANG_TIDY
# for automatic static analysis during builds.
#
# The checks are configured in the .clang-tidy file at the project root.

find_program(CLANG_TIDY_COMMAND
    NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16 clang-tidy-15
    DOC "Path to clang-tidy executable"
)

if(NOT CLANG_TIDY_COMMAND)
    message(WARNING "ENABLE_CLANG_TIDY is ON but clang-tidy was not found!")
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE)
else()
    message(STATUS "Found clang-tidy: ${CLANG_TIDY_COMMAND}")

    # Build the clang-tidy command line
    # Note: WarningsAsErrors is configured in .clang-tidy file for fine-grained control
    set(CLANG_TIDY_ARGS
        "${CLANG_TIDY_COMMAND}"
    )

    # Platform-specific workarounds
    if(MSVC)
        # MSVC cl.exe compatibility: clang-tidy needs /EHsc passed through
        # See: https://gitlab.kitware.com/cmake/cmake/-/issues/20512
        list(APPEND CLANG_TIDY_ARGS "--extra-arg=/EHsc")
    endif()

    # Use compile_commands.json if available (better accuracy)
    if(CMAKE_EXPORT_COMPILE_COMMANDS)
        list(APPEND CLANG_TIDY_ARGS "-p" "${CMAKE_BINARY_DIR}")
    endif()

    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}" CACHE STRING "" FORCE)

    message(STATUS "clang-tidy configured with: ${CMAKE_CXX_CLANG_TIDY}")
endif()
