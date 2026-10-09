// Batteries: the module sandbox, Hash, sql.format, the http
// module and the Copas pump. HTTP runs against a Copas server in the same
// Lua state, so every request also exercises the Copas pump; no network
// beyond 127.0.0.1 is used.
#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

#include "fake_server.hpp"
#include "runtime/runtime.hpp"

namespace vcmp_lua::test {
namespace {

// A directory on package.path for the tests, deleted afterwards.
class ModuleDir {
public:
    ModuleDir() {
        static int counter = 0;
        path_ = std::filesystem::temp_directory_path() /
                ("vcmp_lua_modules_" + std::to_string(++counter));
        std::filesystem::create_directories(path_);
    }
    ~ModuleDir() { std::filesystem::remove_all(path_); }
    ModuleDir(const ModuleDir&) = delete;
    ModuleDir& operator=(const ModuleDir&) = delete;

    void Write(const std::string& name, const std::string& content) const {
        std::ofstream(path_ / name, std::ios::binary) << content;
    }
    [[nodiscard]] std::string PackagePath() const { return (path_ / "?.lua").generic_string(); }

private:
    std::filesystem::path path_;
};

// Runs frames, in real time, until expression is true or seconds pass.
bool FramesUntil(FakeServer& server, const std::string& expression, double seconds = 10) {
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(static_cast<int>(seconds * 1000));
    while (std::chrono::steady_clock::now() < deadline) {
        server.Frame(5);
        if (server.Eval(expression) == "true") {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

// A small HTTP/1.1 server on Copas: answers "<method> <path> <body>", sends
// X-Twice twice, and never answers /hang. Sets the global `port`.
constexpr const char* kCopasServer = R"lua(
    local copas = require "copas"
    local socket = require "socket"
    -- A backlog for 100 simultaneous connections: Windows drops SYNs to a full
    -- backlog, and the client retries only after seconds.
    local listener = assert(socket.bind("127.0.0.1", 0, 128))
    port = select(2, listener:getsockname())
    served = 0
    copas.addserver(listener, function(raw)
      local sock = copas.wrap(raw)
      local line = sock:receive("*l")
      if not line then return end
      local method, path = line:match("^(%u+) (%S+)")
      local length, headers = 0, {}
      while true do
        local header = sock:receive("*l")
        if not header or header == "" then break end
        local name, value = header:match("^([^:]+):%s*(.*)$")
        headers[name:lower()] = value
        if name:lower() == "content-length" then length = tonumber(value) end
      end
      local body = length > 0 and sock:receive(length) or ""
      if path == "/hang" then
        copas.pause(60)
        return
      end
      local reply = method .. " " .. path .. " " .. body .. " " .. tostring(headers["x-test"])
      if method == "HEAD" then reply = "" end
      served = served + 1
      sock:send("HTTP/1.1 200 OK\r\nContent-Length: " .. #reply ..
        "\r\nX-Twice: a\r\nX-Twice: b\r\nConnection: close\r\n\r\n" .. reply)
    end)
)lua";

}  // namespace

TEST_CASE("sandbox: text-mode loading only, no C modules from disk") {
    ModuleDir dir;
    dir.Write("plain.lua", "return { answer = 42 }");
    Config config;
    config.package_path = dir.PackagePath();
    FakeServer server;
    REQUIRE(server.Load(config));
    server.Initialise();

    CHECK(server.Run(R"lua(
        assert(require("plain").answer == 42)

        -- Bytecode is refused everywhere.
        local dumped = string.dump(function() return 1 end)
        local fn, err = load(dumped)
        assert(fn == nil and err:find("binary chunk"), err)
        fn, err = load(dumped, "=x", "b")
        assert(fn == nil and err:find("binary chunk"), err)
        local file = assert(io.open(package.path:gsub("%?", "compiled"), "wb"))
        file:write(dumped)
        file:close()
        fn, err = loadfile((package.path:gsub("%?", "compiled")))
        assert(fn == nil and err:find("binary chunk"), err)
        local ok, message = pcall(dofile, (package.path:gsub("%?", "compiled")))
        assert(not ok and message:find("binary chunk"), message)
        ok, message = pcall(require, "compiled")
        assert(not ok and message:find("error loading module 'compiled'", 1, true)
          and message:find("binary chunk"), message)

        -- load keeps an explicit nil environment apart from none.
        assert(load("return 7")() == 7)
        assert(load("return x", "=c", "t", { x = 5 })() == 5)
        assert(not pcall(load("return x", "=c", "t", nil)))

        -- The console is never read.
        ok, message = pcall(dofile)
        assert(not ok and message:find("reading the console is disabled"), message)
        ok, message = pcall(loadfile)
        assert(not ok and message:find("reading the console is disabled"), message)

        -- No C modules from disk.
        assert(package.cpath == "")
        assert(#package.searchers == 3)
        ok, message = pcall(package.loadlib, "x.so", "luaopen_x")
        assert(not ok and message:find("package.loadlib is disabled"), message)
        ok, message = pcall(require, "no_such_module")
        assert(not ok and message:find("C modules cannot be loaded from disk", 1, true)
          and message:find("no field package.preload['no_such_module']", 1, true), message)
    )lua") == "");
}

TEST_CASE("every built-in module is preloaded, and nothing internal is") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    CHECK(server.Run(R"lua(
        for _, name in ipairs { "lfs", "cjson", "cjson.safe", "socket", "socket.core", "socket.http",
            "mime", "ltn12", "luasql.sqlite3", "luasql.postgres", "luasql.mysql", "copas",
            "copas.http", "inspect", "coxpcall", "sql", "http", "hash" } do
          assert(package.preload[name], name .. " is not preloaded")
          assert(require(name) ~= nil, name)
        end
        assert(package.preload["vcmp-lua/prelude"] == nil)
        assert(require("hash") == Hash)
    )lua") == "");
}

TEST_CASE("Hash: 2.x digests and the OpenSSL functions") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    CHECK(server.Run(R"lua(
        assert(Hash.MD5("abc") == "900150983cd24fb0d6963f7d28e17f72")
        assert(Hash.SHA1("abc") == "a9993e364706816aba3e25717850c26c9cd0d89d")
        assert(Hash.SHA256("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        assert(Hash.SHA512("abc") == "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
          .. "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f")
        assert(Hash.Whirlpool("abc") == "4e2448a4c6f486bb16b6562c73b4020bf3043e3a731bce721ae1b303d97e6d4c"
          .. "7181eebdb6c57e277d0e34957114cbd6c797fc9d95d8b582d225292076d4eef5")
        assert(Hash.SHA256("a\0b") ~= Hash.SHA256("a"), "embedded NUL")
        for _, name in ipairs { "KMAC256", "SKEIN256", "SKEIN512" } do
          local digest = Hash[name]("key", "abc")
          assert(#digest == 128 and digest == Hash[name]("key", "abc"), name)
          assert(digest ~= Hash[name]("other key", "abc"), name)
        end

        assert(Hash.hmac("sha256", "key", "The quick brown fox jumps over the lazy dog") ==
          "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8")
        -- RFC 6070 and RFC 7914 test vectors.
        assert(Hash.pbkdf2("password", "salt", 1, 20, "sha1") ==
          "0c60c80f961f0e71f3a9b524af6012062fe037a6")
        assert(Hash.pbkdf2("password", "salt", 4096, 20, "sha1") ==
          "4b007901b765489abead49d926f721d065a429c1")
        assert(#Hash.pbkdf2("password", "salt", 10, 32) == 64)
        assert(Hash.scrypt("", "", 16, 1, 1, 64) ==
          "77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede2144"
          .. "2fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906")

        local a, b = Hash.randomBytes(16), Hash.randomBytes(16)
        assert(#a == 16 and #b == 16 and a ~= b)
        assert(Hash.randomBytes(0) == "")
        assert(Hash.toHex("\0\255a") == "00ff61")
        assert(Hash.equals("abc", "abc") and not Hash.equals("abc", "abd") and not Hash.equals("a", "ab"))

        local function fails(pattern, fn, ...)
          local ok, err = pcall(fn, ...)
          assert(not ok and err:find(pattern, 1, true), tostring(err))
        end
        fails("unknown digest 'nope'", Hash.hmac, "nope", "k", "d")
        fails("N must be a power of two", Hash.scrypt, "p", "s", 3, 1, 1, 16)
        fails("too much memory", Hash.scrypt, "p", "s", 1 << 20, 8, 1, 16)
        fails("out of range", Hash.pbkdf2, "p", "s", 0, 16)
        fails("out of range", Hash.randomBytes, -1)
    )lua") == "");
}

TEST_CASE("luasql.sqlite3: a query with no rows fetches nil") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    CHECK(server.Run(R"lua(
        local env = assert(require("luasql.sqlite3").sqlite3())
        local conn = assert(env:connect(":memory:"))
        assert(conn:execute("CREATE TABLE t (a INTEGER, b TEXT)"))
        local cursor = assert(conn:execute("SELECT a, b FROM t"))
        assert(cursor:fetch({}, "a") == nil, "an empty result has no row")
        assert(conn:execute("INSERT INTO t VALUES (1, 'x')"))
        cursor = assert(conn:execute("SELECT a, b FROM t"))
        local row = cursor:fetch({}, "a")
        assert(row.a == 1 and row.b == "x")
        assert(cursor:fetch({}, "a") == nil)
        cursor = assert(conn:execute("SELECT a FROM t WHERE a = 2"))
        assert(cursor:fetch() == nil)
        conn:close()
        env:close()
    )lua") == "");
}

TEST_CASE("sql.format fills placeholders with escaped literals") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    CHECK(server.Run(R"lua(
        local sql = require "sql"
        local env = assert(require("luasql.sqlite3").sqlite3())
        local conn = assert(env:connect(":memory:"))

        assert(sql.format(conn, "SELECT ?, ?, ?, ?, ?, ?", nil, true, false, 42, 1.5, "it's") ==
          "SELECT NULL, TRUE, FALSE, 42, 1.5, 'it''s'")
        assert(sql.format(conn, "SELECT ?", math.mininteger) == "SELECT " .. math.mininteger)
        assert(sql.format(conn, "SELECT ?", 0.1) == "SELECT 0.10000000000000001")
        -- Placeholders inside literals, quoted names and comments stay.
        assert(sql.format(conn, "SELECT '?', 'a''?', \"?\", `?` -- ?\n, ? /* ? */", 1) ==
          "SELECT '?', 'a''?', \"?\", `?` -- ?\n, 1 /* ? */")
        assert(sql.format(conn, "SELECT ?? , ?", 2) == "SELECT ? , 2")
        assert(sql.format(conn, "SELECT 1") == "SELECT 1")

        local function fails(pattern, ...)
          local ok, err = pcall(sql.format, ...)
          assert(not ok and err:find(pattern, 1, true), tostring(err))
        end
        fails("more placeholders than the 1 values given", conn, "SELECT ?, ?", 1)
        fails("more placeholders than the 0 values given", conn, "SELECT ?")
        fails("2 values for 1 placeholders", conn, "SELECT ?", 1, 2)
        fails("value 1 is nan", conn, "SELECT ?", 0 / 0)
        fails("value 1 is inf", conn, "SELECT ?", math.huge)
        fails("value 2 is a table", conn, "SELECT ?, ?", 1, {})
        fails("never closed", conn, "SELECT 'abc")
        fails("must be the SQL text", conn, 5)
        fails("must be a LuaSQL connection", nil, "SELECT 1")

        -- Values survive a round trip through the database.
        assert(conn:execute("CREATE TABLE t (id INTEGER, name TEXT, score REAL, flag INTEGER)"))
        local name = "O'Brien \"the\" ; DROP TABLE t; --"
        assert(conn:execute(sql.format(conn, "INSERT INTO t VALUES (?, ?, ?, ?)",
          9007199254740993, name, 0.1, true)) == 1)
        local cursor = assert(conn:execute(sql.format(conn,
          "SELECT id, name, score, flag FROM t WHERE name = ?", name)))
        local id, got, score, flag = cursor:fetch()
        assert(id == 9007199254740993 and got == name and score == 0.1 and flag == 1,
          tostring(id) .. " " .. tostring(got) .. " " .. tostring(score))
        cursor:close()
        conn:close()
        env:close()
    )lua") == "");
}

#ifdef _WIN32
TEST_CASE("LuaSocket on Windows: socket.select takes more than 64 sockets") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    CHECK(server.Eval("require('socket')._SETSIZE") == "1024");
}
#endif

TEST_CASE("Copas: the frame pump steps it and sets copas.running") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    REQUIRE(server.Run(R"lua(
        local copas = require "copas"
        ran = 0
        copas.addthread(function()
          while true do
            ran = ran + 1
            copas.pause(0)
          end
        end)
    )lua") == "");
    server.Frame();
    server.Frame();
    server.Frame();
    CHECK(server.Eval("ran >= 2") == "true");
    CHECK(server.Eval("require('copas').running") == "true");
}

TEST_CASE("http: requests against a local server") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    REQUIRE(server.Run(kCopasServer) == "");
    REQUIRE(server.Run(R"lua(
        local http = require "http"
        results = {}
        local function store(key)
          return function(res, err) results[key] = { res = res, err = err } end
        end
        local base = "http://127.0.0.1:" .. port
        http.request(base .. "/get", store("get"))
        http.request({ url = base .. "/post", body = "hello", headers = { ["X-Test"] = 5 } },
          store("post"))
        http.request({ url = base .. "/put", method = "put", body = "" }, store("put"))
        http.request({ url = base .. "/head", method = "HEAD" }, store("head"))
        http.request({ url = base .. "/hang", timeout = 0.3 }, store("timeout"))
        -- Nothing listens on the listener's port once it is closed: use port 1.
        http.request("http://127.0.0.1:1/", store("refused"))
        expected = 6
    )lua") == "");
    REQUIRE(FramesUntil(server, "(function() local n = 0 for _ in pairs(results) do n = n + 1 end "
                                "return n == expected end)()"));
    CHECK(server.Run(R"lua(
        local get = results.get.res
        assert(get and results.get.err == nil, tostring(results.get.err))
        assert(get.status == 200 and get.body == "GET /get  nil", get.body)
        assert(get.headers["x-twice"] == "a, b" and get.headers["content-length"] == "13",
          get.headers["x-twice"])
        assert(get.url:find("/get$"))
        assert(results.post.res.body == "POST /post hello 5", results.post.res.body)
        assert(results.put.res.body == "PUT /put  nil", results.put.res.body)
        assert(results.head.res.status == 200 and results.head.res.body == "")
        assert(results.timeout.res == nil and results.timeout.err:find("timed out"),
          tostring(results.timeout.err))
        assert(results.refused.res == nil and type(results.refused.err) == "string")
    )lua") == "");
}

TEST_CASE("http: 100 concurrent requests, and callbacks that start requests") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    REQUIRE(server.Run(kCopasServer) == "");
    REQUIRE(server.Run(R"lua(
        local http = require "http"
        ok_count, failures, chained = 0, {}, false
        local base = "http://127.0.0.1:" .. port
        for i = 1, 100 do
          http.request(base .. "/n" .. i, function(res, err)
            if res and res.body == "GET /n" .. i .. "  nil" then
              ok_count = ok_count + 1
            else
              failures[#failures + 1] = tostring(err or res.body)
            end
            if i == 100 then
              http.request(base .. "/chained", function(res2) chained = res2 ~= nil end)
            end
          end)
        end
    )lua") == "");
    const bool done = FramesUntil(server, "ok_count + #failures == 100 and chained", 30);
    INFO(server.Eval("string.format('ok %d, failed %d, chained %s, served %d: %s', ok_count, "
                     "#failures, tostring(chained), served, table.concat(failures, '; '))"));
    REQUIRE(done);
    CHECK(server.Eval("table.concat(failures, '; ')") == "");
    CHECK(server.Eval("served") == "101");
}

TEST_CASE("http: invalid requests raise errors") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    CHECK(server.Run(R"lua(
        local http = require "http"
        local function fails(pattern, ...)
          local ok, err = pcall(http.request, ...)
          assert(not ok and err:find(pattern, 1, true), tostring(err))
        end
        local f = function() end
        fails("must start with http:// or https://", "ftp://example.com/", f)
        fails("must start with http:// or https://", "example.com", f)
        fails("unknown field 'tiemout'", { url = "http://x/", tiemout = 1 }, f)
        fails("field 'url' must be a string", { url = 5 }, f)
        fails("a GET request has no body", { url = "http://x/", method = "GET", body = "b" }, f)
        fails("invalid method 'GE T'", { url = "http://x/", method = "GE T" }, f)
        fails("contains a line break", { url = "http://x/", headers = { A = "b\r\nC: d" } }, f)
        fails("invalid header name 'A B'", { url = "http://x/", headers = { ["A B"] = "c" } }, f)
        fails("field 'timeout' must be between 0", { url = "http://x/", timeout = 0 }, f)
        fails("field 'redirects' must be an integer", { url = "http://x/", redirects = 1.5 }, f)
        fails("argument 2 must be a function", "http://x/")
        fails("table or URL expected", 5, f)
    )lua") == "");
}

TEST_CASE("http: pending requests are dropped on reload and shutdown, without callbacks") {
    FakeServer server;
    REQUIRE(server.Load());
    server.Initialise();
    REQUIRE(server.Run(kCopasServer) == "");
    REQUIRE(server.Run(R"lua(
        local http = require "http"
        http.request("http://127.0.0.1:" .. port .. "/hang", function() record("called back") end)
        Server.reload()
    )lua") == "");
    server.Frame();  // the reload runs at the end of the frame
    REQUIRE(server.Run(kCopasServer) == "");
    REQUIRE(server.Run(R"lua(
        require("http").request("http://127.0.0.1:" .. port .. "/hang",
          function() record("called back") end)
    )lua") == "");
    for (int i = 0; i < 20; ++i) {
        server.Frame();
    }
    server.Shutdown();
    CHECK(server.records.empty());
}

}  // namespace vcmp_lua::test
