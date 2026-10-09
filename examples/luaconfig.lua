-- VCMP-Lua configuration. Put this file in the server directory, next to
-- server.cfg.
-- It runs in a sandbox (no io, os or require) and returns a table.
return {
  -- Scripts to run, in order, when the server starts and on Server.reload().
  -- Event handlers run in the order they were bound, so accounts.lua sees
  -- /register and /login first and cancels them.
  scripts = {
    "lua/accounts.lua",
    "lua/commands.lua",
    "lua/webhook.lua",
  },

  -- Where require looks for your own Lua modules.
  package_path = "lua/?.lua;lua/?/init.lua",

  log = {
    level = "info",            -- trace, debug, info, warn, error, critical, off
    -- file = "logs/vcmp-lua.log",
    -- daily = true,           -- a new file every day
  },

  http = {
    -- CA certificates (PEM) for require "http". Default: the Windows
    -- certificate store; on Linux the distribution's bundle, else a
    -- built-in copy of Mozilla's.
    -- cafile = "certs/ca.pem",
  },
}
