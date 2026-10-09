# Unit tests: the core library against FakeServer, an in-memory server API.
find_package(doctest CONFIG REQUIRED)

add_executable(vcmp_lua_unit_tests
    main.cpp
    fake_server.cpp
    test_args.cpp
    test_entities.cpp
    test_events.cpp
    test_lifecycle.cpp
    test_lua.cpp
    test_timers.cpp)
target_link_libraries(vcmp_lua_unit_tests PRIVATE vcmp_lua_core doctest::doctest)
target_compile_definitions(vcmp_lua_unit_tests PRIVATE
    VCMP_LUA_LUA_TESTS="${PROJECT_SOURCE_DIR}/tests/lua")
vcmp_lua_first_party(vcmp_lua_unit_tests)

add_test(NAME unit COMMAND vcmp_lua_unit_tests)
set_tests_properties(unit PROPERTIES LABELS unit TIMEOUT 300)
