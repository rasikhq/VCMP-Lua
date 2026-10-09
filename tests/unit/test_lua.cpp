// Runs the Lua tests in tests/lua: every file except helpers.lua, each in a
// new FakeServer with helpers.lua loaded first. A file reports failures by
// calling test(name, fn) (helpers.lua); every failing test is listed.
#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "fake_server.hpp"

namespace vcmp_lua::test {
namespace {

std::string ReadFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

std::vector<std::filesystem::path> TestFiles() {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(VCMP_LUA_LUA_TESTS)) {
        if (entry.path().extension() == ".lua" && entry.path().filename() != "helpers.lua") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace

TEST_CASE("Lua tests") {
    const std::filesystem::path dir(VCMP_LUA_LUA_TESTS);
    const std::string helpers = ReadFile(dir / "helpers.lua");
    const auto files = TestFiles();
    REQUIRE_FALSE(files.empty());
    for (const auto& path : files) {
        const std::string name = path.filename().string();
        CAPTURE(name);
        FakeServer server;
        REQUIRE(server.Load());
        server.Initialise();
        REQUIRE(server.Run(helpers, "@helpers.lua") == "");
        CHECK(server.Run(ReadFile(path), "@" + name) == "");
        const std::string failures = server.Eval("table.concat(failures, '\\n\\n')");
        CHECK_MESSAGE(failures.empty(), failures);
        CHECK(server.Eval("#failures") == "0");
    }
}

}  // namespace vcmp_lua::test
