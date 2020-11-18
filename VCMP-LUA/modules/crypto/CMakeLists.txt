cmake_minimum_required(VERSION 3.10)

set(PROJECT_NAME "module-crypto")

add_library(${PROJECT_NAME} STATIC)

target_include_directories(${PROJECT_NAME} PUBLIC
    include
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")