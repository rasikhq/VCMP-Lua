-- Host smoke test: every built-in module loads and works inside the plugin.
--
-- Writes smoke.result ("PASS", or "FAIL" and the failures) in the working
-- directory. A finalizer writes gc.result when the plugin closes the Lua
-- state at shutdown. plugin_host checks both files after unloading.

local function write_file(path, text)
  local file = assert(io.open(path, "w"))
  file:write(text, "\n")
  file:close()
end

local failures = {}

local function check(name, fn)
  local ok, err = pcall(fn)
  if ok then
    print("ok   " .. name)
  else
    print("FAIL " .. name .. ": " .. tostring(err))
    failures[#failures + 1] = name .. ": " .. tostring(err)
  end
end

local function requires(name)
  check(name, function()
    assert(require(name) ~= nil, "module returned nil")
  end)
end

check("lfs", function()
  local lfs = require "lfs"
  assert(type(lfs.currentdir()) == "string")
  assert(lfs.attributes(".", "mode") == "directory")
end)

check("cjson", function()
  local cjson = require "cjson"
  assert(cjson.encode({ 1, 2, 3 }) == "[1,2,3]")
  local decoded = cjson.decode('{"id":9007199254740993,"name":"\\u00e9"}')
  assert(math.type(decoded.id) == "integer" and decoded.id == 9007199254740993)
  assert(decoded.name == "\u{e9}")
  assert(not pcall(cjson.encode, 0 / 0), "NaN must not be encoded")
end)

check("cjson.safe", function()
  local cjson_safe = require "cjson.safe"
  local value, err = cjson_safe.decode("{bad json")
  assert(value == nil and type(err) == "string")
end)

check("socket", function()
  local socket = require "socket"
  assert(socket.gettime() > 0)
  local server = assert(socket.bind("127.0.0.1", 0))
  local _, port = server:getsockname()
  local client = assert(socket.connect("127.0.0.1", port))
  local peer = assert(server:accept())
  assert(client:send("ping\n"))
  assert(peer:receive("*l") == "ping")
  client:close()
  peer:close()
  server:close()
end)

check("socket.url", function()
  local url = require "socket.url"
  local parsed = url.parse("http://user@example.com:8080/a/b?q=1")
  assert(parsed.host == "example.com" and parsed.port == "8080" and parsed.query == "q=1")
end)

check("ltn12", function()
  local ltn12 = require "ltn12"
  local out = {}
  local sink = ltn12.sink.table(out)
  assert(ltn12.pump.all(ltn12.source.string("abc"), sink))
  assert(table.concat(out) == "abc")
end)

check("mime", function()
  local mime = require "mime"
  assert(mime.b64("hello") == "aGVsbG8=")
end)

requires("socket.http")
requires("socket.headers")
requires("socket.tp")
requires("socket.ftp")
requires("socket.smtp")

check("luasql.sqlite3", function()
  local driver = require "luasql.sqlite3"
  local env = assert(driver.sqlite3())
  local conn = assert(env:connect(":memory:"))
  assert(conn:execute("CREATE TABLE t (id INTEGER, name TEXT)"))
  assert(conn:execute("INSERT INTO t VALUES (9007199254740993, 'x')"))
  local cursor = assert(conn:execute("SELECT id, name FROM t"))
  local id, name = cursor:fetch()
  assert(id == 9007199254740993 and name == "x", "got " .. tostring(id))
  cursor:close()
  conn:close()
  env:close()
end)

check("luasql.postgres", function()
  local driver = require "luasql.postgres"
  local env = assert(driver.postgres())
  -- Nothing listens on port 1: the connection fails cleanly.
  local conn, err = env:connect("host=127.0.0.1 port=1 dbname=x user=x connect_timeout=5")
  assert(conn == nil and type(err) == "string", "expected a connection error")
  env:close()
end)

check("luasql.mysql", function()
  local driver = require "luasql.mysql"
  local env = assert(driver.mysql())
  local conn, err = env:connect("x", "x", "x", "127.0.0.1", 1, nil, nil, { connect_timeout = 5 })
  assert(conn == nil and type(err) == "string", "expected a connection error")
  local ok, message = pcall(env.connect, env, "x", "x", "x", "127.0.0.1", 1, nil, nil,
    { conect_timeout = 5 })
  assert(not ok and message:find("unknown connection option", 1, true), tostring(message))
  env:close()
end)

check("copas", function()
  local copas = require "copas"
  local ran = false
  copas.addthread(function()
    copas.pause(0)
    ran = true
  end)
  for _ = 1, 10 do
    copas.step(0)
    if ran then break end
  end
  assert(ran, "the Copas thread did not run")
end)

requires("copas.ftp")
requires("copas.future")
requires("copas.http")
requires("copas.lock")
requires("copas.queue")
requires("copas.semaphore")
requires("copas.smtp")
requires("copas.timer")
requires("binaryheap")
requires("timerwheel")
requires("coxpcall")

check("inspect", function()
  local inspect = require "inspect"
  assert(inspect({ a = 1 }) == "{\n  a = 1\n}")
end)

-- Runs while the plugin closes the Lua state (Runtime::Shutdown).
smoke_finalizer = setmetatable({}, {
  __gc = function()
    print("finalizer runs at shutdown")
    write_file("gc.result", "PASS")
  end,
})

if #failures == 0 then
  write_file("smoke.result", "PASS")
  print("SMOKE PASS")
else
  write_file("smoke.result", "FAIL\n" .. table.concat(failures, "\n"))
  print("SMOKE FAIL")
end
