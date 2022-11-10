project(lua C)

set(srcDir upstream)

set(srcFiles lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c lctype.c ldblib.c ldebug.c ldo.c
    ldump.c lfunc.c lgc.c linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c
    loslib.c lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltests.c ltm.c lundump.c
    lutf8lib.c lvm.c lzio.c)

set(publicHeaderFiles lauxlib.h lua.h luaconf.h lualib.h)

list(TRANSFORM srcFiles PREPEND ${srcDir}/)
list(TRANSFORM publicHeaderFiles PREPEND ${srcDir}/)

add_library(lua
    ${srcFiles})

target_compile_options(lua
    PRIVATE
    $<$<OR:$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:GNU>>:
    -Wextra -Wshadow -Wsign-compare -Wundef -Wwrite-strings -Wredundant-decls
    -Wdisabled-optimization -Waggregate-return -Wdouble-promotion -Wdeclaration-after-statement
    -Wmissing-prototypes -Wnested-externs -Wstrict-prototypes -Wc++-compat -Wold-style-definition>)

set(includeDir ${CMAKE_CURRENT_BINARY_DIR}/include)
file(COPY ${publicHeaderFiles} DESTINATION ${includeDir})

add_library(lua-header
    INTERFACE)

target_include_directories(lua-header
    INTERFACE
    ${includeDir})

target_link_libraries(lua
    INTERFACE
    lua-header)

add_library(lua::lib ALIAS lua)
add_library(lua::header ALIAS lua-header)
