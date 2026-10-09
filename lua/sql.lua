-- require "sql": builds SQL text with values for LuaSQL, which has no
-- prepared statements.
--
--   local sql = require "sql"
--   conn:execute(sql.format(conn, "SELECT * FROM users WHERE name = ? AND age > ?", name, 18))
--
-- Each ? outside a quoted string, quoted name or comment takes the next
-- value; ?? is a literal question mark. Values become:
--   nil            NULL
--   true, false    TRUE, FALSE
--   integer        its digits
--   float          17 significant digits (NaN and infinities are refused)
--   string         a quoted literal, escaped by conn:escape() for that
--                  connection's server and character set
-- Anything else is an error, and so is a value count that does not match
-- the placeholders.
--
-- Quotes in the SQL text are found the standard way: a quote ends a string
-- unless it is doubled ('it''s'). Backslash escapes and Postgres
-- dollar-quoted strings are not recognised, so write those literals as a
-- placeholder value instead.

local error, select, tostring, type = error, select, tostring, type
local math_type, huge = math.type, math.huge
local format = string.format
local concat = table.concat

local sql = {}

local function literal(conn, value, index)
  local kind = type(value)
  if value == nil then
    return "NULL"
  elseif kind == "boolean" then
    return value and "TRUE" or "FALSE"
  elseif kind == "number" then
    if math_type(value) == "integer" then
      return tostring(value)
    end
    if value ~= value or value == huge or value == -huge then
      error(format("sql.format: value %d is %s, which SQL cannot store", index, tostring(value)), 3)
    end
    return format("%.17g", value)
  elseif kind == "string" then
    local escaped, err = conn:escape(value)
    if type(escaped) ~= "string" then
      error(format("sql.format: cannot escape value %d: %s", index, tostring(err)), 3)
    end
    return "'" .. escaped .. "'"
  end
  error(format("sql.format: value %d is a %s, which is not a SQL value", index, kind), 3)
end

-- The position of the character that closes what opens at start, or an error.
local function skip_quoted(template, start, quote)
  local close = start
  while true do
    close = template:find(quote, close + 1, true)
    if not close then
      error(format("sql.format: the %s at position %d is never closed", quote, start), 3)
    end
    if template:sub(close + 1, close + 1) ~= quote then
      return close
    end
    close = close + 1 -- a doubled quote stays inside
  end
end

function sql.format(conn, template, ...)
  if type(template) ~= "string" then
    error(format("sql.format: argument 2 must be the SQL text (got %s)", type(template)), 2)
  end
  if type(conn) ~= "userdata" and type(conn) ~= "table" then
    error(format("sql.format: argument 1 must be a LuaSQL connection (got %s)", type(conn)), 2)
  end
  local count = select("#", ...)
  local values = { ... }
  local out = {}
  local used = 0
  local pos, length = 1, #template
  while pos <= length do
    local start = template:find("[?'\"`/-]", pos)
    if not start then
      out[#out + 1] = template:sub(pos)
      break
    end
    out[#out + 1] = template:sub(pos, start - 1)
    local char = template:sub(start, start)
    local after = template:sub(start + 1, start + 1)
    local stop = start
    if char == "?" then
      if after == "?" then
        out[#out + 1] = "?"
        stop = start + 1
      else
        used = used + 1
        if used > count then
          error(format("sql.format: more placeholders than the %d values given", count), 2)
        end
        out[#out + 1] = literal(conn, values[used], used)
      end
    elseif char == "'" or char == '"' or char == "`" then
      stop = skip_quoted(template, start, char)
      out[#out + 1] = template:sub(start, stop)
    elseif char == "-" and after == "-" then
      stop = template:find("\n", start, true) or length
      out[#out + 1] = template:sub(start, stop)
    elseif char == "/" and after == "*" then
      local _, close = template:find("*/", start + 2, true)
      stop = close or length
      out[#out + 1] = template:sub(start, stop)
    else
      out[#out + 1] = char
    end
    pos = stop + 1
  end
  if used ~= count then
    error(format("sql.format: %d values for %d placeholders", count, used), 2)
  end
  return concat(out)
end

return sql
