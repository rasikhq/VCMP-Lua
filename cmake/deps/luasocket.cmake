# LuaSocket 3.1.0: socket.core and mime.core in C, the rest in Lua.
vcmp_lua_fetch(luasocket
    URL https://github.com/lunarmodules/luasocket/archive/refs/tags/v3.1.0.tar.gz
    SHA256 bf033aeb9e62bcaa8d007df68c119c966418e8c9ef7e4f2d7e96bddeca9cca6e)

set(_socket_src "${luasocket_SOURCE_DIR}/src")
set(_socket_sources
    auxiliar.c buffer.c compat.c except.c inet.c io.c luasocket.c mime.c
    options.c select.c tcp.c timeout.c udp.c)
if(WIN32)
    list(APPEND _socket_sources wsocket.c)
    set(_socket_libraries ws2_32)
else()
    list(APPEND _socket_sources usocket.c)
    set(_socket_libraries "")
endif()
list(TRANSFORM _socket_sources PREPEND "${_socket_src}/")

# Winsock's fd_set holds 64 sockets by default, so socket.select (and Copas)
# would refuse a 65th; on Windows FD_SETSIZE is a count, not a highest fd.
vcmp_lua_c_module(vcmp_lua_luasocket
    SOURCES ${_socket_sources}
    INCLUDE_DIRS "${_socket_src}"
    DEFINITIONS LUASOCKET_API= $<$<C_COMPILER_ID:MSVC>:_WINSOCK_DEPRECATED_NO_WARNINGS>
        $<$<BOOL:${WIN32}>:FD_SETSIZE=1024>
    LIBRARIES ${_socket_libraries})

vcmp_lua_embed_lua(
    socket         "${_socket_src}/socket.lua"
    socket.ftp     "${_socket_src}/ftp.lua"
    socket.headers "${_socket_src}/headers.lua"
    socket.http    "${_socket_src}/http.lua"
    socket.smtp    "${_socket_src}/smtp.lua"
    socket.tp      "${_socket_src}/tp.lua"
    socket.url     "${_socket_src}/url.lua"
    ltn12          "${_socket_src}/ltn12.lua"
    mime           "${_socket_src}/mime.lua")
