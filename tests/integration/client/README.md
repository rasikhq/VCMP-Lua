# Real-client test (phase 3)

`client.lua` checks the player-facing bindings and events with a real
VC:MP 0.4 client, which the unit tests (fake server) and
`tests/integration/bindings` (no players) cannot.

## Set up a Windows server

1. Get `LuaPlugin_x64.dll` from the CI run's `LuaPlugin_x64-windows`
   artifact.
2. In the directory of the 64-bit VC:MP 0.4 Windows server:

   ```
   plugins/LuaPlugin_x64.dll
   client.lua                 (this directory's file)
   luaconfig.lua              (below)
   server.cfg                 (below)
   ```

   `luaconfig.lua`:

   ```lua
   return {
     scripts = { "client.lua" },
     log = { level = "info", file = "logs/vcmp-lua.log" },
   }
   ```

   `server.cfg` (keep your other settings):

   ```
   gamemode VCMP-Lua P3 test
   port 8192
   maxplayers 10
   plugins LuaPlugin_x64
   ```

3. Start the server. The console shows
   `[client] ready: join the server; /help after spawning`.

## Test

1. Join with the client (`127.0.0.1:8192` on the same PC). The console
   logs every event with its arguments (`[event] onPlayerConnect(Player(0))`).
2. Pick a class and spawn. A second later the automatic checks run; each
   prints PASS or FAIL in the chat and the console, then a total.
3. Type `/help` and run the commands one by one: `/car`, `/eject`, `/cp`
   (walk out and back in), `/pickup` (walk into it), `/obj` (touch it and
   shoot it), `/bind` (press K), `/cam`, `/args a b  c`, `/name NewName`,
   `/stats`.
4. Last: `/reload` (scripts reload, the vehicles, objects, pickups,
   checkpoints and binds the script made disappear; you stay connected),
   then `/kick` (the console logs `onPlayerDisconnect(..., 2)`).
5. Stop the server with Ctrl+C; it should exit without a crash.

Send back `logs/vcmp-lua.log` (or the console output) and anything that
looked wrong in the game.
