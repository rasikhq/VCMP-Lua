# Throwaway probe plugin (docs/internals.md). Standalone project, not
# part of the main build; needs only the SDK header:
#
#   cmake -S tests/probe -B build/probe -G Ninja && cmake --build build/probe
cmake_minimum_required(VERSION 3.25)
project(vcmp_lua_probe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(probe IN ITEMS probe_a probe_b)
    add_library(${probe} MODULE probe.cpp)
    set_target_properties(${probe} PROPERTIES PREFIX "" CXX_VISIBILITY_PRESET hidden)
    target_include_directories(${probe} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/vcmp")
    target_compile_definitions(${probe} PRIVATE PROBE_NAME="${probe}")
    target_link_options(${probe} PRIVATE -static-libstdc++ -static-libgcc)
endforeach()
target_compile_definitions(probe_b PRIVATE PROBE_ACTIVE=0)
