-- Loads the example scripts (examples/lua) and calls their handlers with
-- stand-in players: tables with the members the examples use.
for _, f in ipairs({ "lua/accounts.lua", "lua/commands.lua", "lua/webhook.lua" }) do
  dofile(f)
end

local failures = 0

local function mock(name)
  local p = { name = name, data = {}, world = 0, angle = 0, health = 50, messages = {} }
  p.position = { 1, 2, 3 }
  function p:msg(text)
    self.messages[#self.messages + 1] = text
    print(name .. " <- " .. text)
  end
  return p
end

local function expect(cond, what)
  if cond then
    print("ok   " .. what)
  else
    failures = failures + 1
    print("FAIL " .. what)
  end
end

Timer.create(function()
  local ok, err = pcall(function()
    local a = mock("Alice")
    Event.trigger("onPlayerConnect", a)
    expect(a.messages[1]:find("register"), "welcome")
    Event.trigger("onPlayerCommand", a, "register", { "secret" }, "secret")
    expect(a.data.loggedIn == true, "registered")
    expect(#a.messages == 2, "register not seen by commands.lua")
    local b = mock("Alice")
    Event.trigger("onPlayerCommand", b, "login", { "wrong" }, "wrong")
    expect(b.data.loggedIn == nil, "wrong password refused")
    Event.trigger("onPlayerCommand", b, "login", { "secret" }, "secret")
    expect(b.data.loggedIn == true, "login")
    Event.trigger("onPlayerCommand", a, "car", { "191" }, "191")
    expect(a.data.car and a.data.car.valid, "car created")
    local first = a.data.car
    Event.trigger("onPlayerCommand", a, "car", { "130" }, "130")
    expect(not first.valid and a.data.car.valid, "old car replaced")
    Event.trigger("onPlayerCommand", a, "car", nil, "")
    expect(a.messages[#a.messages]:find("usage"), "car usage")
    Event.trigger("onPlayerCommand", a, "pos", nil, "")
    expect(a.messages[#a.messages]:find("x = 1.00"), "pos")
    Event.trigger("onPlayerCommand", a, "heal", nil, "")
    expect(a.health == 100, "heal")
    Event.trigger("onPlayerCommand", a, "nope", nil, "")
    expect(a.messages[#a.messages]:find("unknown command"), "unknown")
    Event.trigger("onPlayerDisconnect", a, 1)
    expect(not a.data.car.valid, "car deleted on disconnect")
  end)
  expect(ok, "no error: " .. tostring(err))
  print(failures == 0 and "EXAMPLES PASS" or "EXAMPLES FAIL")
  Server.shutdown()
end, 100, 1)
