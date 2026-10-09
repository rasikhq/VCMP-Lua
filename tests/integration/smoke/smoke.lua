-- Smoke test in a real VC:MP 0.4 server (tests/integration/compose.yml):
--
-- Stage 1, with the default CA certificates:
--   - CRUD through LuaSQL and sql.format on SQLite, Postgres 17, MySQL 8.4
--     and MariaDB 11, then 1,000 inserts per database in one frame;
--   - timers, and creating and destroying entities;
--   - HTTPS to the local TLS server fails: its CA is not trusted.
-- Then it writes a luaconfig.lua that trusts the test CA (http.cafile) and
-- reloads. Stage 2:
--   - the vehicle stage 1 left behind is gone (reload deletes it);
--   - HTTPS works with the custom CA, a wrong host name fails, and 100
--     concurrent requests succeed;
--   - prints SMOKE PASS (or SMOKE FAIL) and shuts the server down; a
--     finalizer prints "smoke finalizer ran" when the Lua state closes.

local cjson = require "cjson"
local http = require "http"
local socket = require "socket"
local sql = require "sql"

local STATE_FILE = "smoke-state.json"
local CA_FILE = "/certs/ca.pem"
local TLS_URL = "https://tls-server:8443"
local WRONG_HOST_URL = "https://tls-wrong:8443"

local failures = {}

local function fail(name, err)
  failures[#failures + 1] = name .. ": " .. tostring(err)
  print("FAIL " .. name .. ": " .. tostring(err))
end

local function check(name, fn)
  local started = socket.gettime()
  local ok, err = pcall(fn)
  if ok then
    print(("ok   %s (%.2f s)"):format(name, socket.gettime() - started))
  else
    fail(name, err)
  end
end

local function eq(actual, expected, what)
  if actual ~= expected then
    error(("%s: expected %s, got %s"):format(what, tostring(expected), tostring(actual)), 2)
  end
end

local function write_file(path, text)
  local file = assert(io.open(path, "w"))
  file:write(text)
  file:close()
end

local function read_state()
  local file = io.open(STATE_FILE)
  if not file then
    return { stage = 1 }
  end
  local state = cjson.decode(file:read("a"))
  file:close()
  return state
end

-- Asynchronous checks: each calls done(err) once; finish() runs when the
-- last one is done. The stage itself holds one count while it starts them.
local pending = 1
local finish

local function settle()
  pending = pending - 1
  if pending == 0 then finish() end
end

local function async(name, start)
  pending = pending + 1
  local finished = false
  local function done(err)
    if finished then return end
    finished = true
    if err then fail(name, err) else print("ok   " .. name) end
    settle()
  end
  local ok, err = pcall(start, done)
  if not ok then done(err) end
end

-- Databases

local mysql_timeouts = { connect_timeout = 10, read_timeout = 30, write_timeout = 30 }

local DATABASES = {
  {
    name = "sqlite",
    connect = function()
      local env = assert(require("luasql.sqlite3").sqlite3())
      return env, assert(env:connect("smoke.sqlite3"))
    end,
  },
  {
    name = "postgres17",
    connect = function()
      local env = assert(require("luasql.postgres").postgres())
      return env, assert(env:connect(
        "host=postgres17 dbname=smoke user=smoke password=smoke-password connect_timeout=10"))
    end,
  },
  {
    name = "mysql84",
    connect = function()
      local env = assert(require("luasql.mysql").mysql())
      local options = { ssl = "require" }
      for key, value in pairs(mysql_timeouts) do options[key] = value end
      return env, assert(env:connect("smoke", "smoke", "smoke-password", "mysql84", 3306, nil, nil,
        options))
    end,
  },
  {
    name = "mariadb11",
    connect = function()
      local env = assert(require("luasql.mysql").mysql())
      return env, assert(env:connect("smoke", "smoke", "smoke-password", "mariadb11", 3306, nil, nil,
        mysql_timeouts))
    end,
  },
}

local function first_row(conn, template, ...)
  local cursor = assert(conn:execute(sql.format(conn, template, ...)))
  local row = { cursor:fetch() }
  cursor:close()
  return table.unpack(row)
end

local function crud(conn)
  assert(conn:execute("DROP TABLE IF EXISTS smoke_items"))
  assert(conn:execute(
    "CREATE TABLE smoke_items (id BIGINT PRIMARY KEY, name VARCHAR(100), score DOUBLE PRECISION)"))
  local quoted = "O'Brien \"the\" back\\slash; DROP TABLE smoke_items; --"
  eq(conn:execute(sql.format(conn, "INSERT INTO smoke_items VALUES (?, ?, ?), (?, ?, ?)",
    1, "first", 1.5, 9007199254740993, quoted, nil)), 2, "inserted rows")
  local id, name, score = first_row(conn, "SELECT id, name, score FROM smoke_items WHERE id = ?",
    9007199254740993)
  eq(tostring(id), "9007199254740993", "64-bit id")
  eq(name, quoted, "escaped name")
  eq(score, nil, "NULL")
  eq(conn:execute(sql.format(conn, "UPDATE smoke_items SET name = ? WHERE id = ?", "renamed", 1)), 1,
    "updated rows")
  eq(first_row(conn, "SELECT name FROM smoke_items WHERE id = ?", 1), "renamed", "updated name")
  eq(conn:execute(sql.format(conn, "DELETE FROM smoke_items WHERE id = ?", 1)), 1, "deleted rows")
  eq(tostring(first_row(conn, "SELECT COUNT(*) FROM smoke_items")), "1", "rows left")
end

local function burst(conn)
  assert(conn:execute("DROP TABLE IF EXISTS smoke_burst"))
  assert(conn:execute("CREATE TABLE smoke_burst (id INTEGER PRIMARY KEY, name VARCHAR(20))"))
  -- One transaction: 1,000 commits would wait for 1,000 disk flushes.
  assert(conn:setautocommit(false))
  for i = 1, 1000 do
    eq(conn:execute(sql.format(conn, "INSERT INTO smoke_burst VALUES (?, ?)", i, "row" .. i)), 1,
      "row " .. i)
  end
  assert(conn:commit())
  assert(conn:setautocommit(true))
  eq(tostring(first_row(conn, "SELECT COUNT(*) FROM smoke_burst")), "1000", "burst rows")
end

local function databases()
  for _, db in ipairs(DATABASES) do
    local env, conn
    check(db.name .. ": connect and CRUD", function()
      env, conn = db.connect()
      crud(conn)
    end)
    if conn then
      check(db.name .. ": 1,000 inserts in one frame", function() burst(conn) end)
      conn:close()
    end
    if env then env:close() end
  end
end

-- Stage 1

local function stage1()
  -- One timer callback is one server frame.
  async("databases", function(done)
    Timer.create(function()
      databases()
      done()
    end, 0, 1)
  end)

  async("timers", function(done)
    local fired = 0
    local started = socket.gettime()
    Timer.create(function()
      fired = fired + 1
      if fired == 5 then
        local elapsed = socket.gettime() - started
        done(elapsed < 0.09 and ("5 x 20 ms took only %.3f s"):format(elapsed) or nil)
      end
    end, 20, 5)
  end)

  check("entities: create and destroy", function()
    local vehicle = Vehicle.create(191, 0, -1000, 1000, 10, 0)
    eq(vehicle.valid, true, "vehicle.valid")
    eq(Vehicle.findByID(vehicle.id), vehicle, "findByID")
    vehicle:destroy()
    eq(vehicle.valid, false, "vehicle.valid after destroy")
    local object = Object.create(600, 0, -1000, 1000, 10)
    eq(object.valid, true, "object.valid")
    object:destroy()
    eq(object.valid, false, "object.valid after destroy")
  end)

  async("https: an untrusted CA is refused", function(done)
    http.request(TLS_URL .. "/hello", function(res, err)
      if res then return done("connected without trusting the test CA") end
      print("     " .. err)
      -- Not a CA bundle problem: the default bundle loaded and refused it.
      done(not err:find("unable to get local issuer certificate", 1, true) and err or nil)
    end)
  end)

  finish = function()
    -- Left for the reload to delete.
    local vehicle = Vehicle.create(191, 0, -1000, 1000, 10, 0)
    write_file(STATE_FILE, cjson.encode({ stage = 2, vehicle = vehicle.id, failures = failures }))
    write_file("luaconfig.lua", ([[
return {
  scripts = { "smoke.lua" },
  log = { level = "debug" },
  http = { cafile = %q },
}
]]):format(CA_FILE))
    print("SMOKE stage 1 done; reloading with http.cafile")
    Server.reload()
  end
end

-- Stage 2

local function stage2(state)
  for _, failure in ipairs(state.failures) do
    failures[#failures + 1] = "stage 1: " .. failure
  end

  check("reload deleted the vehicle the old scripts created", function()
    eq(Vehicle.findByID(state.vehicle), nil, "Vehicle.findByID(" .. state.vehicle .. ")")
  end)

  async("https: custom CA from the config", function(done)
    http.request({ url = TLS_URL .. "/hello", timeout = 10 }, function(res, err)
      if not res then return done(err) end
      done(not (res.status == 200 and res.body == "hello /hello") and res.body or nil)
    end)
  end)

  async("https: a wrong host name is refused", function(done)
    http.request(WRONG_HOST_URL .. "/hello", function(res, err)
      if res then return done("connected to a host the certificate does not name") end
      print("     " .. err)
      done(not err:find("tls-wrong", 1, true) and err or nil)
    end)
  end)

  async("https: 100 concurrent requests", function(done)
    local left, errors = 100, {}
    local started = socket.gettime()
    for i = 1, 100 do
      http.request({ url = TLS_URL .. "/n" .. i, timeout = 60 }, function(res, err)
        if not (res and res.body == "hello /n" .. i) then
          errors[#errors + 1] = tostring(err or res.body)
        end
        left = left - 1
        if left == 0 then
          print(("     100 requests in %.2f s"):format(socket.gettime() - started))
          done(#errors > 0 and table.concat(errors, "; ") or nil)
        end
      end)
    end
  end)

  finish = function()
    smoke_finalizer = setmetatable({}, {
      __gc = function() print("smoke finalizer ran") end,
    })
    if #failures == 0 then
      print("SMOKE PASS")
    else
      print("SMOKE FAIL\n" .. table.concat(failures, "\n"))
    end
    Timer.create(function() Server.shutdown() end, 100, 1)
  end
end

Event.bind("onServerInit", function()
  local state = read_state()
  print("SMOKE stage " .. state.stage)
  Timer.create(function()
    if pending > 0 then
      fail("deadline", pending .. " checks did not finish in 600 s")
      pending = 0
      finish()
    end
  end, 600000, 1)
  if state.stage == 1 then stage1() else stage2(state) end
  settle()
end)
