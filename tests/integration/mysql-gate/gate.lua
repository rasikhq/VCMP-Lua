-- MySQL gate (plan, Part C, P1): runs inside the plugin and decides whether
-- MySQL/MariaDB support stays in v2. It must show that
--   - caching_sha2_password (statically linked) works against MySQL 8.4,
--   - mysql_native_password works against MariaDB 11,
--   - no authentication plugin is loaded from disk (run.sh plants fake ones),
--   - the connect/read/write timeouts of the LuaSQL patch work.
-- Writes mysql-gate.result: "PASS" or "FAIL", then the details.

local socket = require "socket"
local driver = require "luasql.mysql"

local env = assert(driver.mysql())
local failures = {}
local notes = {}

local function note(text)
  print("     " .. text)
  notes[#notes + 1] = text
end

local function check(name, fn)
  local started = socket.gettime()
  local ok, err = pcall(fn)
  local elapsed = socket.gettime() - started
  if ok then
    print(("ok   %s (%.1f s)"):format(name, elapsed))
  else
    print(("FAIL %s (%.1f s): %s"):format(name, elapsed, tostring(err)))
    failures[#failures + 1] = name .. ": " .. tostring(err)
  end
end

local function connect(user, password, host, options)
  return env:connect("gate", user, password, host, 3306, nil, nil, options)
end

local function first_value(conn, sql)
  local cursor = assert(conn:execute(sql))
  local value = cursor:fetch()
  cursor:close()
  return value
end

local function tls_cipher(conn)
  local cursor = assert(conn:execute("SHOW SESSION STATUS LIKE 'Ssl_cipher'"))
  local _, value = cursor:fetch()
  cursor:close()
  return value or ""
end

local timeouts = { connect_timeout = 10, read_timeout = 30, write_timeout = 30 }

local function with(options)
  local merged = {}
  for key, value in pairs(timeouts) do merged[key] = value end
  for key, value in pairs(options) do merged[key] = value end
  return merged
end

-- MySQL 8.4 -----------------------------------------------------------------

check("mysql84 caching_sha2_password: full authentication over TLS", function()
  local conn = assert(connect("sha2_tls", "gate-sha2-tls", "mysql84", with({ ssl = "require" })))
  assert(first_value(conn, "SELECT CURRENT_USER()") == "sha2_tls@%")
  local cipher = tls_cipher(conn)
  assert(cipher ~= "", "the connection does not use TLS")
  note("mysql84: " .. first_value(conn, "SELECT VERSION()") .. ", TLS cipher " .. cipher)
  conn:close()
end)

check("mysql84 caching_sha2_password: fast authentication (cached account)", function()
  local conn = assert(connect("sha2_tls", "gate-sha2-tls", "mysql84", with({ ssl = "require" })))
  assert(first_value(conn, "SELECT CURRENT_USER()") == "sha2_tls@%")
  conn:close()
end)

check("mysql84 caching_sha2_password: full authentication over plain TCP (RSA key exchange)", function()
  local conn = assert(connect("sha2_rsa", "gate-sha2-rsa", "mysql84", with({ ssl = "disable" })))
  assert(first_value(conn, "SELECT CURRENT_USER()") == "sha2_rsa@%")
  assert(tls_cipher(conn) == "", "expected a plain connection")
  conn:close()
end)

check("mysql84 with default TLS settings: the self-signed certificate is refused", function()
  local conn, err = connect("sha2_tls", "gate-sha2-tls", "mysql84", timeouts)
  if conn then
    conn:close()
    error("connected although the server certificate cannot be verified")
  end
  note("mysql84, default TLS settings: " .. err)
end)

check("mysql84 queries: integers, NULL, escaping", function()
  local conn = assert(connect("sha2_tls", "gate-sha2-tls", "mysql84", with({ ssl = "require" })))
  assert(conn:execute("CREATE TABLE IF NOT EXISTS t (id BIGINT PRIMARY KEY, name VARCHAR(64) NULL)"))
  assert(conn:execute("DELETE FROM t"))
  local name = conn:escape("it's \"quoted\"")
  assert(conn:execute(("INSERT INTO t VALUES (9007199254740993, '%s'), (2, NULL)"):format(name)))
  local cursor = assert(conn:execute("SELECT id, name FROM t ORDER BY id"))
  local id, value = cursor:fetch()
  assert(id == "2" and value == nil, "row 1: " .. tostring(id) .. ", " .. tostring(value))
  id, value = cursor:fetch()
  assert(id == "9007199254740993" and value == "it's \"quoted\"", "row 2: " .. tostring(id))
  cursor:close()
  conn:close()
end)

-- MariaDB 11 ----------------------------------------------------------------

check("mariadb11 mysql_native_password: default TLS settings", function()
  local conn = assert(connect("native", "gate-native", "mariadb11", timeouts))
  assert(first_value(conn, "SELECT CURRENT_USER()") == "native@%")
  local cipher = tls_cipher(conn)
  assert(cipher ~= "", "the connection does not use TLS")
  note("mariadb11: " .. first_value(conn, "SELECT VERSION()") .. ", TLS cipher " .. cipher)
  conn:close()
end)

-- client_ed25519 cannot be linked statically. The connector looks for it
-- on disk before it checks the certificate (run.sh plants fake plugins).
check("mariadb11 ed25519: refused cleanly, no plugin loaded from disk", function()
  local conn, err = connect("ed25519", "gate-ed25519", "mariadb11", with({ ssl = "require" }))
  if conn then
    conn:close()
    error("connected with ed25519, which is not built in")
  end
  assert(err:find("client_ed25519", 1, true), err)
  note("mariadb11, ed25519 account: " .. err)
  conn, err = connect("ed25519", "gate-ed25519", "mariadb11", timeouts)
  assert(conn == nil, "connected with ed25519 and default TLS settings")
  note("mariadb11, ed25519 account, default TLS settings: " .. err)
end)

local client_plugins = {
  caching_sha2_password = true, client_ed25519 = true, dialog = true, mysql_clear_password = true,
  mysql_native_password = true, sha256_password = true, zstd = true,
}

check("no client plugin is mapped into the process", function()
  local maps = assert(io.open("/proc/self/maps")):read("a")
  for path in maps:gmatch("%s(/%S+)\n") do
    local name = path:match("([^/]+)%.so$")
    if name and client_plugins[name] then
      error("mapped: " .. path)
    end
  end
end)

-- Timeouts (LuaSQL patch) ---------------------------------------------------

local function timed(fn)
  local started = socket.gettime()
  local a, b = fn()
  return socket.gettime() - started, a, b
end

check("connect_timeout: a server that accepts and never answers", function()
  local elapsed, conn, err = timed(function()
    return connect("x", "x", "tarpit", { connect_timeout = 2, read_timeout = 2 })
  end)
  assert(conn == nil, "connected to the tarpit")
  assert(elapsed >= 1.5 and elapsed < 10, ("gave up after %.1f s"):format(elapsed))
  note(("tarpit: gave up after %.1f s: %s"):format(elapsed, err))
end)

check("read_timeout: a query that runs too long", function()
  local conn = assert(connect("sha2_timeout", "gate-timeout", "mysql84",
    { ssl = "require", connect_timeout = 10, read_timeout = 2 }))
  local elapsed, cursor, err = timed(function() return conn:execute("SELECT SLEEP(20)") end)
  assert(cursor == nil, "the query was not interrupted")
  assert(elapsed >= 1.5 and elapsed < 12, ("gave up after %.1f s"):format(elapsed))
  note(("SELECT SLEEP(20) with read_timeout = 2: gave up after %.1f s: %s"):format(elapsed, err))
  conn:close()
end)

check("write_timeout: accepted", function()
  local conn = assert(connect("sha2_timeout", "gate-timeout", "mysql84",
    { ssl = "require", connect_timeout = 10, write_timeout = 5 }))
  assert(first_value(conn, "SELECT 1") == "1")
  conn:close()
end)

check("invalid options are rejected", function()
  local invalid = {
    { connect_timeout = 0 }, { read_timeout = "five" }, { write_timeout = 1e9 },
    { ssl = "maybe" }, { ssl_ca = 1 }, { conect_timeout = 5 },
  }
  for _, options in ipairs(invalid) do
    local ok = pcall(connect, "x", "x", "mysql84", options)
    assert(not ok, "accepted option " .. tostring(next(options)))
  end
end)

env:close()

local file = assert(io.open("mysql-gate.result", "w"))
file:write(#failures == 0 and "PASS\n" or "FAIL\n")
for _, failure in ipairs(failures) do file:write("failed: ", failure, "\n") end
for _, text in ipairs(notes) do file:write(text, "\n") end
file:close()
print(#failures == 0 and "MYSQL GATE SCRIPT PASS" or "MYSQL GATE SCRIPT FAIL")
