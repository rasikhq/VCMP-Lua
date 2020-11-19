cmake_minimum_required(VERSION 3.10)

### Vendor
add_subdirectory(vendor/spdlog)
add_subdirectory(vendor/Lua)
###

### Modules
#add_subdirectory(modules/crypto)
add_subdirectory(modules/SQLiteCPP)
add_subdirectory(modules/mariadb-connector-cpp)
add_subdirectory(modules/cpr)
###

set(PROJECT_NAME "Lua_Plugin")

add_library(${PROJECT_NAME} SHARED
    pch.h
    Core.cpp

    include/ConvertUTF.c include/ConvertUTF.h
    include/SimpleIni.h
    include/utility.h
    include/vcmp.h

    vcmpWrap/Constants.h
    vcmpWrap/globalTables.h
    vcmpWrap/vcmpCallbacks.h

    vcmpWrap/Classes/Bind.cpp       vcmpWrap/Classes/Bind.h
    vcmpWrap/Classes/Checkpoint.cpp vcmpWrap/Classes/Checkpoint.h
    vcmpWrap/Classes/Object.cpp     vcmpWrap/Classes/Object.h
    vcmpWrap/Classes/Pickup.cpp     vcmpWrap/Classes/Pickup.h
    vcmpWrap/Classes/Player.cpp     vcmpWrap/Classes/Player.h
    vcmpWrap/Classes/Server.cpp     vcmpWrap/Classes/Server.h
    vcmpWrap/Classes/Stream.cpp     vcmpWrap/Classes/Stream.h
    vcmpWrap/Classes/Vehicle.cpp    vcmpWrap/Classes/Vehicle.h

    vcmpWrap/EventManager/EventManager.cpp  vcmpWrap/EventManager/EventManager.h

    vcmpWrap/Log/Logger.h

    vcmpWrap/Timer/TimerManager.cpp vcmpWrap/Timer/TimerManager.h
    vcmpWrap/Timer/vcmpTimer.cpp    vcmpWrap/Timer/vcmpTimer.h

    # Modules
    vcmpWrap/Modules/Crypto/Hash.cpp                vcmpWrap/Modules/Crypto/Hash.h
    
    vcmpWrap/Modules/SQLite3/SqLite.cpp             vcmpWrap/Modules/SQLite3/SqLite.h
    
    vcmpWrap/Modules/MariaDB/MySQL.cpp              vcmpWrap/Modules/MariaDB/MySQL.h
    vcmpWrap/Modules/MariaDB/MySQLAccount.cpp       vcmpWrap/Modules/MariaDB/MySQLAccount.h
    vcmpWrap/Modules/MariaDB/MySQLConnection.cpp    vcmpWrap/Modules/MariaDB/MySQLConnection.h

    vcmpWrap/Modules/CPR/Remote.cpp                 vcmpWrap/Modules/CPR/Remote.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/VCMP-LUA
    include

    vcmpWrap
        vcmpWrap/Classes
        vcmpWrap/EventManager
        vcmpWrap/Log
        vcmpWrap/Timer
        vcmpWrap/Modules

    modules/crypto/include

    vendor
    vendor/sol
    vendor/spdlog
)

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "LuaPlugin")

# Modules
target_link_libraries(${PROJECT_NAME} PUBLIC module-crypto)
target_link_libraries(${PROJECT_NAME} PUBLIC SQLiteCpp)
# target_link_libraries(${PROJECT_NAME} PUBLIC mariadbclientpp::mariadbclientpp)
target_link_libraries(${PROJECT_NAME} PUBLIC mariadb_connector_cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC cpr::cpr)

# Vendor
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog)
target_link_libraries(${PROJECT_NAME} PUBLIC Lua)