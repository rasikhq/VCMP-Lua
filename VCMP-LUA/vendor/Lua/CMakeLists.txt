project(Lua)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB src *.c *.h *.hpp)

add_library(Lua STATIC ${src})