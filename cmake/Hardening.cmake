# Compiler and linker settings shared by the targets this project builds.

# Hardening flags for everything compiled here, third-party sources included.
function(vcmp_lua_harden target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /guard:cf)
        target_link_options(${target} PRIVATE /guard:cf /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT)
    else()
        # _FORTIFY_SOURCE needs an optimised build.
        target_compile_options(${target} PRIVATE
            -fstack-protector-strong
            "$<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE;-D_FORTIFY_SOURCE=2>")
        target_link_options(${target} PRIVATE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack)
    endif()
endfunction()

# Our own code builds warning-free (plan B3.10). Third-party code does not get
# these flags: its warnings are not ours to fix.
function(vcmp_lua_first_party target)
    vcmp_lua_harden(${target})
    if(MSVC)
        # C4702 (unreachable code) is a backend warning: it is raised inside
        # sol2's headers when a binding always throws, e.g. the stubs of the
        # removed v1 globals, and v1's ported switches return before break.
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /utf-8 /bigobj /Zc:__cplusplus
            /wd4702)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
        # The sanitizers' instrumentation makes GCC report false positives
        # here (in sol2's headers); the release build keeps the warning.
        if(VCMP_LUA_SANITIZE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE -Wno-maybe-uninitialized)
        endif()
    endif()
endfunction()

# Link rules for the plugin binary itself (plan B8).
function(vcmp_lua_plugin_link target)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(map "${PROJECT_SOURCE_DIR}/cmake/plugin.map")
        target_link_options(${target} PRIVATE
            -static-libstdc++
            -static-libgcc
            -Wl,-z,defs
            "-Wl,--version-script=${map}"
            -Wl,--exclude-libs,ALL
            -Wl,--as-needed)
        set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${map}")
    elseif(MSVC)
        target_link_options(${target} PRIVATE /OPT:REF /OPT:ICF)
    endif()
endfunction()
