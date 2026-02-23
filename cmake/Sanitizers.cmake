# Sanitizers.cmake — Cross-platform sanitizer configuration
#
# Provides: target_enable_sanitizers(<target>)
#
# Reads the following cache variables (set via option() or presets):
#   SANITIZE_ADDRESS, SANITIZE_MEMORY, SANITIZE_THREAD, SANITIZE_UNDEFINED
#
# Sanitizer compatibility:
#   - ASan + UBSan: Compatible, recommended combination
#   - ASan + TSan: NOT compatible (mutually exclusive)
#   - ASan + MSan: NOT compatible (mutually exclusive)
#   - TSan + MSan: NOT compatible (mutually exclusive)

# Validate sanitizer combinations at configure time
if(ENABLE_SANITIZERS)
    if(SANITIZE_ADDRESS AND SANITIZE_THREAD)
        message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be used together. "
            "Disable one of SANITIZE_ADDRESS or SANITIZE_THREAD.")
    endif()

    if(SANITIZE_ADDRESS AND SANITIZE_MEMORY)
        message(FATAL_ERROR "AddressSanitizer and MemorySanitizer cannot be used together. "
            "Disable one of SANITIZE_ADDRESS or SANITIZE_MEMORY.")
    endif()

    if(SANITIZE_THREAD AND SANITIZE_MEMORY)
        message(FATAL_ERROR "ThreadSanitizer and MemorySanitizer cannot be used together. "
            "Disable one of SANITIZE_THREAD or SANITIZE_MEMORY.")
    endif()

    # Log which sanitizers are enabled
    set(_enabled_sanitizers "")
    if(SANITIZE_ADDRESS)
        list(APPEND _enabled_sanitizers "AddressSanitizer")
    endif()
    if(SANITIZE_MEMORY)
        list(APPEND _enabled_sanitizers "MemorySanitizer")
    endif()
    if(SANITIZE_THREAD)
        list(APPEND _enabled_sanitizers "ThreadSanitizer")
    endif()
    if(SANITIZE_UNDEFINED)
        list(APPEND _enabled_sanitizers "UndefinedBehaviorSanitizer")
    endif()

    if(_enabled_sanitizers)
        list(JOIN _enabled_sanitizers ", " _sanitizer_list)
        message(STATUS "Sanitizers enabled: ${_sanitizer_list}")
    endif()
endif()

function(target_enable_sanitizers TARGET)
    if(NOT ENABLE_SANITIZERS)
        return()
    endif()

    get_target_property(_type ${TARGET} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    # Collect sanitizer flags
    set(_compile_flags "")
    set(_link_flags "")

    # --- AddressSanitizer ---
    if(SANITIZE_ADDRESS)
        if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # MSVC ASan doesn't support debug runtime (-MDd), only release runtime (-MD)
            # In Debug builds, we must switch to release runtime or disable ASan
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                message(WARNING "MSVC AddressSanitizer requires release runtime (-MD) but Debug build uses -MDd. "
                    "Disabling ASan for ${TARGET}. Use clang-cl (Windows LLVM) for Debug+ASan.")
            else()
                list(APPEND _compile_flags /fsanitize=address)
            endif()
            # MSVC doesn't need link flags for ASan
        elseif(MSVC)
            # clang-cl (MSVC-compatible Clang)
            list(APPEND _compile_flags /fsanitize=address)
        else()
            list(APPEND _compile_flags -fsanitize=address -fno-omit-frame-pointer)
            list(APPEND _link_flags -fsanitize=address)

            # On macOS, use shared ASan runtime
            if(APPLE)
                list(APPEND _compile_flags -shared-libasan)
                list(APPEND _link_flags -shared-libasan)
            endif()
        endif()
    endif()

    # --- ThreadSanitizer ---
    if(SANITIZE_THREAD)
        # TSan requires compiler support - clang-cl on Windows doesn't support it
        if(MSVC OR WIN32)
            message(WARNING "ThreadSanitizer is not supported on Windows — ignoring SANITIZE_THREAD for ${TARGET}")
        else()
            list(APPEND _compile_flags -fsanitize=thread)
            list(APPEND _link_flags -fsanitize=thread)
        endif()
    endif()

    # --- MemorySanitizer ---
    if(SANITIZE_MEMORY)
        # MSan requires special libc++ builds and only works on Linux
        if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(WARNING "MemorySanitizer is only supported by Clang on Linux — ignoring SANITIZE_MEMORY for ${TARGET}")
        else()
            list(APPEND _compile_flags -fsanitize=memory -fno-omit-frame-pointer)
            list(APPEND _link_flags -fsanitize=memory)
        endif()
    endif()

    # --- UndefinedBehaviorSanitizer ---
    if(SANITIZE_UNDEFINED)
        # MSVC doesn't support UBSan, but clang-cl does
        if(MSVC AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(WARNING "UBSan is not supported by MSVC — ignoring SANITIZE_UNDEFINED for ${TARGET}")
        else()
            list(APPEND _compile_flags -fsanitize=undefined)
            list(APPEND _link_flags -fsanitize=undefined)

            # Optionally trap on UB for immediate crash (helps debugging)
            # list(APPEND _compile_flags -fno-sanitize-recover=undefined)
        endif()
    endif()

    # Apply flags to target
    if(_compile_flags)
        # Add debug symbols for better stack traces (MSVC uses /Zi which is already set in Debug builds)
        if(NOT MSVC)
            list(APPEND _compile_flags -g)
        endif()
        target_compile_options(${TARGET} PUBLIC ${_compile_flags})
    endif()

    if(_link_flags)
        target_link_options(${TARGET} PUBLIC ${_link_flags})
    endif()
endfunction()
