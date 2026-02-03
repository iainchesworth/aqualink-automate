# Sanitizers.cmake — Inline replacement for the external sanitizers-cmake dependency.
#
# Provides: target_enable_sanitizers(<target>)
#
# Reads the following cache variables (set via option() or presets):
#   SANITIZE_ADDRESS, SANITIZE_MEMORY, SANITIZE_THREAD, SANITIZE_UNDEFINED

function(target_enable_sanitizers TARGET)
    if(NOT ENABLE_SANITIZERS)
        return()
    endif()

    get_target_property(_type ${TARGET} TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()

    # --- AddressSanitizer ---
    if(SANITIZE_ADDRESS)
        if(MSVC)
            target_compile_options(${TARGET} PUBLIC /fsanitize=address)
        else()
            target_compile_options(${TARGET} PUBLIC -g -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(${TARGET} PUBLIC -fsanitize=address)
        endif()
    endif()

    # --- ThreadSanitizer ---
    if(SANITIZE_THREAD)
        if(MSVC)
            message(WARNING "ThreadSanitizer is not supported by MSVC — ignoring SANITIZE_THREAD for ${TARGET}")
        else()
            target_compile_options(${TARGET} PUBLIC -g -fsanitize=thread)
            target_link_options(${TARGET} PUBLIC -fsanitize=thread)
        endif()
    endif()

    # --- MemorySanitizer ---
    if(SANITIZE_MEMORY)
        if(MSVC)
            message(WARNING "MemorySanitizer is not supported by MSVC — ignoring SANITIZE_MEMORY for ${TARGET}")
        else()
            target_compile_options(${TARGET} PUBLIC -g -fsanitize=memory)
            target_link_options(${TARGET} PUBLIC -fsanitize=memory)
        endif()
    endif()

    # --- UndefinedBehaviorSanitizer ---
    if(SANITIZE_UNDEFINED)
        if(MSVC)
            message(WARNING "UBSan is not supported by MSVC — ignoring SANITIZE_UNDEFINED for ${TARGET}")
        else()
            target_compile_options(${TARGET} PUBLIC -g -fsanitize=undefined)
            target_link_options(${TARGET} PUBLIC -fsanitize=undefined)
        endif()
    endif()
endfunction()
