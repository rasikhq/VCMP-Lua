set(_generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(runtime/version.hpp.in "${_generated}/runtime/version.hpp" @ONLY)

get_property(_embedded_modules GLOBAL PROPERTY VCMP_LUA_EMBEDDED_MODULES)
vcmp_lua_embed("${_generated}/embedded_modules.cpp"
    MODULES
        ${_embedded_modules}
        coxpcall "${PROJECT_SOURCE_DIR}/lua/coxpcall.lua")

# Everything except the entry point. The plugin and the tests link it.
add_library(vcmp_lua_core STATIC
    bindings/args.cpp
    bindings/bindings.cpp
    bindings/convert.cpp
    bindings/entities.cpp
    bindings/enums.cpp
    bindings/event.cpp
    bindings/player.cpp
    bindings/server.cpp
    bindings/timer.cpp
    bindings/vehicle.cpp
    core/entity_pool.cpp
    core/event_bus.cpp
    core/frame_pump.cpp
    core/scheduler.cpp
    plugin/callbacks.cpp
    runtime/config.cpp
    runtime/errors.cpp
    runtime/libraries.cpp
    runtime/log.cpp
    runtime/preload.cpp
    runtime/runtime.cpp
    "${_generated}/embedded_modules.cpp")
target_include_directories(vcmp_lua_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${_generated}")
target_include_directories(vcmp_lua_core SYSTEM PUBLIC "${PROJECT_SOURCE_DIR}/third_party/vcmp")
# Build-wide definitions (plan B3.10). SOL_ALL_SAFETIES_ON decides whether
# sol::function calls are protected, so it must be the same in every file.
# SOL_PRINT_ERRORS=0: errors reach Lua and the plugin log, not stderr.
target_compile_definitions(vcmp_lua_core PUBLIC SOL_ALL_SAFETIES_ON=1 SOL_PRINT_ERRORS=0)
if(WIN32)
    target_compile_definitions(vcmp_lua_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
target_link_libraries(vcmp_lua_core
    PUBLIC
        lua
        sol2::sol2
        spdlog::spdlog
        fmt::fmt
    PRIVATE
        vcmp_lua_lfs
        vcmp_lua_cjson
        vcmp_lua_luasocket
        vcmp_lua_luasql
        vcmp_lua_digestpp
        CURL::libcurl
        OpenSSL::SSL
        OpenSSL::Crypto
        ZLIB::ZLIB
        unofficial::sqlite3::sqlite3
        PostgreSQL::PostgreSQL
        unofficial::libmariadb)
vcmp_lua_first_party(vcmp_lua_core)

# The plugin: LuaPlugin_x64.so / LuaPlugin_x64.dll, the v1 file name.
add_library(vcmp_lua MODULE plugin/entry.cpp)
set_target_properties(vcmp_lua PROPERTIES
    OUTPUT_NAME LuaPlugin_x64
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "$<1:${PROJECT_BINARY_DIR}/bin>"
    RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_BINARY_DIR}/bin>")
target_link_libraries(vcmp_lua PRIVATE vcmp_lua_core)
vcmp_lua_first_party(vcmp_lua)
vcmp_lua_plugin_link(vcmp_lua)
