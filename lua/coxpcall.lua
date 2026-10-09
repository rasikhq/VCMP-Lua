-- Built-in replacement for the "coxpcall" module.
--
-- coxpcall makes pcall and xpcall coroutine-safe on Lua 5.1. On Lua 5.4 they
-- already are, so the standard functions are returned as they are.
-- timerwheel (required by Copas) requires "coxpcall" on every Lua version.
return {
  pcall = pcall,
  xpcall = xpcall,
  running = coroutine.running,
}
