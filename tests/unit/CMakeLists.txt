# Unit tests: the core library against FakeServer, an in-memory server API.
find_package(doctest CONFIG REQUIRED)

add_executable(vcmp_lua_unit_tests
    main.cpp
    fake_server.cpp
    test_entities.cpp
    test_events.cpp
    test_lifecycle.cpp
    test_timers.cpp)
target_link_libraries(vcmp_lua_unit_tests PRIVATE vcmp_lua_core doctest::doctest)
vcmp_lua_first_party(vcmp_lua_unit_tests)

add_test(NAME unit COMMAND vcmp_lua_unit_tests)
set_tests_properties(unit PROPERTIES LABELS unit TIMEOUT 300)
