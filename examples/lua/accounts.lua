-- Accounts: /register <password> and /login <password>, stored in SQLite
-- with salted PBKDF2 hashes.

local sql = require "sql"

local env = assert(require("luasql.sqlite3").sqlite3())
local db = assert(env:connect("accounts.sqlite3"))
assert(db:execute([[
  CREATE TABLE IF NOT EXISTS accounts (
    name TEXT PRIMARY KEY,
    salt TEXT NOT NULL,
    hash TEXT NOT NULL
  )]]))

-- Hashing runs on the server thread: more iterations are safer but stall
-- the server for longer on each login.
local ITERATIONS = 100000

local function hash_password(password, salt)
  return Hash.pbkdf2(password, salt, ITERATIONS, 32, "sha256")
end

local function find_account(name)
  local cursor = assert(db:execute(sql.format(db,
    "SELECT salt, hash FROM accounts WHERE name = ?", name)))
  local row = cursor:fetch({}, "a")
  cursor:close()
  return row
end

local function register(player, password)
  if find_account(player.name) then
    return player:msg("This name is registered already. Use /login.")
  end
  local salt = Hash.toHex(Hash.randomBytes(16))
  assert(db:execute(sql.format(db, "INSERT INTO accounts (name, salt, hash) VALUES (?, ?, ?)",
    player.name, salt, hash_password(password, salt))))
  player.data.loggedIn = true
  player:msg("Registered and logged in.")
end

local function login(player, password)
  local account = find_account(player.name)
  if not account then
    return player:msg("No account with this name. Use /register.")
  end
  if not Hash.equals(hash_password(password, account.salt), account.hash) then
    return player:msg("Wrong password.")
  end
  player.data.loggedIn = true
  player:msg("Logged in.")
end

Event.bind("onPlayerCommand", function(player, command, args)
  if command ~= "register" and command ~= "login" then
    return
  end
  -- Keep passwords out of other scripts' command handlers.
  Event.cancel()
  if not args then
    return player:msg(("usage: /%s <password>"):format(command))
  end
  if player.data.loggedIn then
    return player:msg("You are logged in already.")
  end
  if command == "register" then
    register(player, args[1])
  else
    login(player, args[1])
  end
end)

Event.bind("onPlayerConnect", function(player)
  player:msg(find_account(player.name) and "Welcome back! Use /login <password>."
    or "Welcome! Use /register <password>.")
end)

Event.bind("onServerShutdown", function()
  db:close()
  env:close()
end)
