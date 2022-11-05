file(GLOB_RECURSE SRC 
	pch.cpp
	Core.cpp
	${CMAKE_CURRENT_LIST_DIR}/include/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/vcmpWrap/*.cpp
)

file(GLOB_RECURSE HEADERS 
	pch.h
	${CMAKE_CURRENT_LIST_DIR}/include/*.h
	${CMAKE_CURRENT_LIST_DIR}/vcmpWrap/*.h
	${CMAKE_CURRENT_LIST_DIR}/vendor/sol/*.hpp
)

add_library(VCMPLua SHARED ${HEADERS} ${SRC})

target_include_directories(VCMPLua PRIVATE 
	${CMAKE_CURRENT_LIST_DIR}/
	${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_LIST_DIR}/vcmpWrap
	${CMAKE_CURRENT_LIST_DIR}/vendor
	${CMAKE_CURRENT_LIST_DIR}/vendor/Lua
	${CMAKE_CURRENT_LIST_DIR}/vendor/sol
	${CMAKE_CURRENT_LIST_DIR}/vendor/spdlog/include
	${CMAKE_CURRENT_LIST_DIR}/vendor/asyncplusplus/include
	${CMAKE_CURRENT_LIST_DIR}/vendor/lanes/src
)

target_link_libraries(VCMPLua Async++)
target_link_libraries(VCMPLua spdlog)
target_link_libraries(VCMPLua Lua)
target_link_libraries(VCMPLua lanes)
# target_link_libraries(VCMPLua mysql-connector-cpp)

# set c++ standard 17
set_target_properties(VCMPLua PROPERTIES
   	CXX_STANDARD 17
)