-- Runs once in every new Lua state, after the standard libraries and the
-- package.preload entries, before the bindings and scripts
-- (src/runtime/runtime.cpp). It applies the module sandbox and returns the
-- functions the frame pump calls.
--
-- Not a security boundary: scripts still have io, os and debug. It keeps
-- scripts from loading bytecode by accident, which can crash Lua 5.4, and
-- from loading C modules from disk, which cannot link to the plugin's
-- hidden copy of Lua (and on Windows would start a second Lua).

local error, type = error, type
local raw_load, raw_loadfile = load, loadfile
local package = package

local MODULES_HELP = "see docs/MIGRATION-v3.md#modules"

-- Text mode only. The variadic tail keeps an explicit nil env apart from a
-- missing one, as load and loadfile do.
function load(chunk, chunkname, _, ...)
  return raw_load(chunk, chunkname, "t", ...)
end

-- loadfile() and dofile() without a file name read the console, which
-- would stop the server.
function loadfile(filename, _, ...)
  if filename == nil then
    error("loadfile: reading the console is disabled; pass a file name", 2)
  end
  return raw_loadfile(filename, "t", ...)
end

function dofile(filename)
  if filename == nil then
    error("dofile: reading the console is disabled; pass a file name", 2)
  end
  local fn, err = raw_loadfile(filename, "t")
  if not fn then
    error(err, 0)
  end
  return fn()
end

package.cpath = ""
package.loadlib = function()
  error("package.loadlib is disabled: C modules must be built into the plugin (" ..
    MODULES_HELP .. ")", 2)
end

-- Searcher 1 (package.preload) stays: it finds the built-in modules.
-- Searcher 2 loads Lua files from package.path in text mode. The C
-- searchers 3 and 4 are replaced by one that explains why.
local searchers = package.searchers

searchers[2] = function(name)
  local filename, err = package.searchpath(name, package.path)
  if not filename then
    return err
  end
  local fn, load_err = raw_loadfile(filename, "t")
  if not fn then
    error(("error loading module '%s' from file '%s':\n\t%s"):format(name, filename, load_err), 0)
  end
  return fn, filename
end

searchers[3] = function()
  return "C modules cannot be loaded from disk (" .. MODULES_HELP .. ")"
end

searchers[4] = nil

-- The Copas pump: one non-blocking step per server frame, once a script has
-- required copas. copas.loop() would block the server, and only it sets
-- copas.running, so the pump sets it.
local loaded = package.loaded

local function copas_step()
  local copas = loaded.copas
  if type(copas) == "table" and type(copas.step) == "function" then
    copas.running = true
    copas.step(0)
  end
end

return {
  copas_step = copas_step,
}
