file(GLOB_RECURSE SRC pch.cpp Core.cpp ${CMAKE_CURRENT_LIST_DIR}/include/*.cpp ${CMAKE_CURRENT_LIST_DIR}/vcmpWrap/*.cpp)
file(GLOB_RECURSE HEADERS pch.h ${CMAKE_CURRENT_LIST_DIR}/include/*.h ${CMAKE_CURRENT_LIST_DIR}/vcmpWrap/*.h)

add_library(VCMPLua SHARED ${HEADERS} ${SRC})

target_include_directories(VCMPLua PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/vcmpWrap)

# set c++ standard 17
set_target_properties(VCMPLua PROPERTIES
   	CXX_STANDARD 17
)