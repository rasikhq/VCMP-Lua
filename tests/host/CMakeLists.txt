# plugin_host loads the built plugin binary (not the core library) with a fake
# server API, on Linux and on Windows.
add_executable(plugin_host plugin_host.cpp)
target_include_directories(plugin_host SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/third_party/vcmp")
target_link_libraries(plugin_host PRIVATE ${CMAKE_DL_LIBS})
if(WIN32)
    target_compile_definitions(plugin_host PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
vcmp_lua_first_party(plugin_host)
add_dependencies(plugin_host vcmp_lua)

# vcmp_lua_host_test(<name> [CONFIG <luaconfig.lua text>] [ARGS <arg>...]
#                    [PASS_REGEX <regex>])
# Runs plugin_host in its own directory, with that luaconfig.lua (or none).
function(vcmp_lua_host_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "CONFIG;PASS_REGEX" "ARGS")
    set(dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    file(MAKE_DIRECTORY "${dir}")
    if(DEFINED arg_CONFIG)
        file(CONFIGURE OUTPUT "${dir}/luaconfig.lua" CONTENT "${arg_CONFIG}" @ONLY)
    else()
        file(REMOVE "${dir}/luaconfig.lua")
    endif()
    add_test(NAME ${name}
        COMMAND plugin_host "$<TARGET_FILE:vcmp_lua>" ${arg_ARGS}
        WORKING_DIRECTORY "${dir}")
    set_tests_properties(${name} PROPERTIES TIMEOUT 120)
    if(DEFINED arg_PASS_REGEX)
        set_tests_properties(${name} PROPERTIES PASS_REGULAR_EXPRESSION "${arg_PASS_REGEX}")
    endif()
endfunction()

set(_smoke_config "return {
  scripts = { \"${CMAKE_CURRENT_SOURCE_DIR}/smoke.lua\" },
  log = { level = \"debug\" },
}
")

# init -> frames -> shutdown -> unload, with every built-in module required.
vcmp_lua_host_test(host_smoke
    CONFIG "${_smoke_config}"
    ARGS --frames 120 --expect smoke.result --expect gc.result)

# A server without the newest callbacks (structSize is smaller).
vcmp_lua_host_test(host_old_server
    CONFIG "${_smoke_config}"
    ARGS --server old --expect smoke.result --expect gc.result)

# A server without OnServerFrame: the plugin refuses to load and writes
# nothing past structSize.
vcmp_lua_host_test(host_ancient_server
    CONFIG "${_smoke_config}"
    ARGS --server ancient --expect-init-failure)

vcmp_lua_host_test(host_missing_config
    ARGS --expect-init-failure)

vcmp_lua_host_test(host_invalid_config
    CONFIG "return { scripts = { \"a.lua\", 42 } }\n"
    ARGS --expect-init-failure
    PASS_REGEX "scripts\\[2\\] must be a string \\(got number\\)(.|\n)*HOST PASS")

# A script that fails to load does not stop the scripts after it.
vcmp_lua_host_test(host_script_error
    CONFIG "return { scripts = { \"missing.lua\", \"${CMAKE_CURRENT_SOURCE_DIR}/smoke.lua\" } }\n"
    ARGS --expect smoke.result)
