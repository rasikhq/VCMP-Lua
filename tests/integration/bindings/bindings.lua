-- Bindings against a real VC:MP 0.4 server, without players: every
-- class is created, read back after writing, and destroyed; then the
-- scripts reload once and the server shuts down. Prints BINDINGS PASS or
-- BINDINGS FAIL with the failed checks. See run.sh.

local failures = {}
local checks = 0

local function check(name, fn)
    checks = checks + 1
    local ok, err = pcall(fn)
    if not ok then
        failures[#failures + 1] = name .. ": " .. tostring(err)
        print("FAIL " .. name .. ": " .. tostring(err))
    end
end

local function eq(actual, expected, what)
    if actual ~= expected then
        error(("%s: expected %s, got %s"):format(what, tostring(expected), tostring(actual)), 2)
    end
end

local function near(actual, expected, what)
    if type(actual) ~= "number" or math.abs(actual - expected) > 1e-3 then
        error(("%s: expected %s, got %s"):format(what, tostring(expected), tostring(actual)), 2)
    end
end

local function vec(actual, x, y, z, what)
    near(actual[1], x, what .. "[1]")
    near(actual[2], y, what .. "[2]")
    near(actual[3], z, what .. "[3]")
end

local function raises(text, fn)
    local ok, err = pcall(fn)
    if ok then error("expected an error containing '" .. text .. "'", 2) end
    if not tostring(err):find(text, 1, true) then
        error("expected an error containing '" .. text .. "', got '" .. tostring(err) .. "'", 2)
    end
end

local reloaded = Server.getGame() == "bindings: reloaded"
print("bindings: run " .. (reloaded and "2 (after reload)" or "1"))

check("no players", function()
    eq(Player.count(), 0, "Player.count()")
    eq(next(Player.getActive()), nil, "Player.getActive()")
end)

check("Server text settings", function()
    Server.name = "VCMP-Lua bindings"
    eq(Server.name, "VCMP-Lua bindings", "name")
    Server.setPassword("pw")
    eq(Server.getPassword(), "pw", "password")
    Server.password = ""
    eq(Server.password, "", "no password")
    Server.maxPlayers = 32
    eq(Server.maxPlayers, 32, "maxPlayers")
    local settings = Server.getSettings()
    eq(settings.serverName, "VCMP-Lua bindings", "getSettings().serverName")
    eq(settings.maxPlayers, 32, "getSettings().maxPlayers")
end)

check("Server world settings", function()
    Server.hour = 13
    eq(Server.hour, 13, "hour")
    Server.minute = 37
    eq(Server.minute, 37, "minute")
    Server.weather = 4
    eq(Server.weather, 4, "weather")
    Server.timeRate = 2000
    eq(Server.timeRate, 2000, "timeRate")
    Server.gravity = 0.01
    near(Server.gravity, 0.01, "gravity")
    Server.gameSpeed = 1.5
    near(Server.gameSpeed, 1.5, "gameSpeed")
    Server.waterLevel = 7
    near(Server.waterLevel, 7, "waterLevel")
    Server.flightAltitude = 150
    near(Server.flightAltitude, 150, "flightAltitude")
    Server.vehicleRespawnHeight = -20
    near(Server.vehicleRespawnHeight, -20, "vehicleRespawnHeight")
    Server.killDelay = 3
    eq(Server.killDelay, 3, "killDelay")
    Server.fallTimer = 500
    eq(Server.fallTimer, 500, "fallTimer")
    Server.wastedSettings = { deathTimer = 4000, fadeInSpeed = 0.5 }
    local w = Server.wastedSettings
    eq(w.deathTimer, 4000, "wastedSettings.deathTimer")
    near(w.fadeInSpeed, 0.5, "wastedSettings.fadeInSpeed")
end)

check("Server options", function()
    Server.setOption(ServerOption.friendlyFire, true)
    eq(Server.getOption(ServerOption.friendlyFire), true, "friendlyFire on")
    Server.setOption(ServerOption.friendlyFire, false)
    eq(Server.getOption(ServerOption.friendlyFire), false, "friendlyFire off")
    Server.setOption(ServerOption.disableCrouch, true)
    eq(Server.getOption(ServerOption.disableCrouch), true, "disableCrouch")
    Server.setOption(ServerOption.disableCrouch, false)
end)

check("Server classes, bans, explosions, names", function()
    local class = Server.addClass(1, 0xFF0000FF, 0, { -1000, 1000, 10, 90 }, 26, 100)
    eq(math.type(class), "integer", "addClass returns an id")
    Server.addClass(2, -1, 1, -1000, 1000, 10, 0)
    Server.setClassPosition(-1000, 1000, 10)
    Server.setClassCameraPosition({ -1000, 1005, 12 })
    Server.setClassCameraLook(-1000, 1000, 10)
    eq(Server.banIP("10.20.30.40"), true, "banIP")
    eq(Server.isIPBanned("10.20.30.40"), true, "isIPBanned")
    eq(Server.banIP("10.20.30.40"), false, "banIP again")
    eq(Server.unbanIP("10.20.30.40"), true, "unbanIP")
    eq(Server.isIPBanned("10.20.30.40"), false, "isIPBanned after unban")
    Server.createExplosion(0, 1, { 0, 0, 10 })
    eq(Server.getSkinName(0), "Tommy Vercetti", "getSkinName")
    eq(Server.getSkinID("cop"), 1, "getSkinID")
    eq(Weapon.getName(26), "M4", "Weapon.getName")
end)

check("Map, Radio, Weapon, Sound", function()
    Map.setBounds(1000, -1000, 2000, -2000)
    local b = Map.getBounds()
    near(b.max_x, 1000, "max_x")
    near(b.min_y, -2000, "min_y")
    Map.setBounds(10000, -10000, 10000, -10000)
    Map.hideObject(1000, -1.5, 2.25, 10)
    Map.showMapObject(1000, -1.5, 2.25, 10)
    Map.showAllObjects()
    eq(Map.getDistrictName(0, 0), "Leaf Links", "getDistrictName")
    eq(Radio.createStream(20, "Test", "http://127.0.0.1/stream", false), true, "createStream")
    eq(Radio.destroyStream(20), true, "destroyStream")
    Weapon.data(26, 1, 200)
    near(Weapon.data(26, 1), 200, "Weapon.data")
    eq(Weapon.isFieldModified(26, 1), true, "isFieldModified")
    Weapon.resetField(26, 1)
    eq(Weapon.isFieldModified(26, 1), false, "isFieldModified after reset")
    Weapon.resetAll()
    Sound.play(50)
    Sound.play(0, 50, { 1, 2, 3 })
end)

check("Vehicle", function()
    local v = Vehicle.create(191, 0, { -1000, 1000, 10, 90 }, 3, 7)
    eq(v.valid, true, "valid")
    eq(v.model, 191, "model")
    eq(Vehicle.findByID(v.id), v, "findByID")
    v.health = 500
    near(v.health, 500, "health")
    v.world = 5
    eq(v.world, 5, "world")
    v.world = 0
    local colour = v.color
    eq(colour[1], 3, "primary colour")
    eq(colour[2], 7, "secondary colour")
    v.color = { 1 }
    eq(v.color[1], 1, "primary colour set")
    eq(v.color[2], 7, "secondary colour kept")
    v.position = { -1001, 1001, 11 }
    vec(v.position, -1001, 1001, 11, "position")
    v.spawnPosition = { -1002, 1002, 12 }
    vec(v.spawnPosition, -1002, 1002, 12, "spawnPosition")
    v.rotation = { 0, 0, 1.5 }
    v.rotation = v.rotation  -- round-trips (v1 could not)
    near(v.rotation.euler[3], 1.5, "rotation z")
    v.idleRespawnTime = 60000
    eq(v.idleRespawnTime, 60000, "idleRespawnTime")
    v.taxiLight = true
    eq(v.taxiLight, true, "taxiLight")
    v.immunity = 0x1F
    eq(v.immunity, 0x1F, "immunity")
    v.radio = 3
    eq(v.radio, 3, "radio")
    v:setOption(VehicleOption.siren, true)
    eq(v:getOption(VehicleOption.siren), true, "siren option")
    v:setPartStatus(0, 1)
    v:setTyreStatus(0, 1)
    v:setSpeed(0, 0, 1)
    eq(type(v:getSpeed()), "table", "getSpeed")
    eq(v:getOccupant(0), nil, "no driver")
    v:setHandlingRule(1, 2000)
    near(v:getHandlingRule(1), 2000, "handling rule")
    eq(v:hasHandlingRule(1), true, "hasHandlingRule")
    v:resetHandling()
    eq(v:hasHandlingRule(1), false, "hasHandlingRule after reset")
    v:repair()
    near(v.health, 1000, "health after repair")
    eq(v.wrecked, false, "wrecked")
    local id = v.id
    collectgarbage()
    eq(Vehicle.findByID(id), v, "still exists after a collection")
    eq(v:destroy(), true, "destroy")
    eq(v.valid, false, "valid after destroy")
    raises("vehicle no longer exists", function() return v.health end)
    raises("'create' failed", function() Vehicle.create(5000, 0, 0, 0, 0, 0) end)
end)

check("Object", function()
    local o = Object.create(600, 0, -1000, 1000, 10)
    eq(o.model, 600, "model")
    o.world = 2
    eq(o.world, 2, "world")
    o.world = 0
    o.alpha = 100
    eq(o:getAlpha(), 100, "alpha")
    o.trackShots = true
    eq(o.trackShots, true, "trackShots")
    o.trackTouch = true
    eq(o.trackTouch, true, "trackTouch")
    o.position = { -1001, 1001, 11 }
    vec(o.position, -1001, 1001, 11, "position")
    o.angle = { 0, 0, 1 }
    near(o.angle[3], 1, "angle set to, not by")
    o.angle = { 0, 0, 1 }
    near(o.angle[3], 1, "angle again")
    o:moveTo(-1000, 1000, 10, 100)
    o:rotateTo({ 0, 0, 0 }, 100)
    eq(o:destroy(), true, "destroy")
end)

check("Pickup", function()
    local k = Pickup.create(366, 0, 5, { -1000, 1000, 10 })
    eq(k.model, 366, "model")
    eq(k.quantity, 5, "quantity")
    k.world = 3
    eq(k.world, 3, "world")
    k.alpha = 128
    eq(k.alpha, 128, "alpha")
    k.auto = false
    eq(k.auto, false, "auto")
    k.autoTimer = 5000
    eq(k.autoTimer, 5000, "autoTimer")
    k.position = { -1001, 1001, 11 }
    vec(k.position, -1001, 1001, 11, "position")
    k:setOption(PickupOption.singleUse, true)
    eq(k:getOption(PickupOption.singleUse), true, "singleUse")
    eq(k:respawn(), true, "respawn")
    eq(k:destroy(), true, "destroy")
end)

check("Checkpoint", function()
    local c = Checkpoint.create(nil, 0, true, { -1000, 1000, 10 }, { 255, 0, 0, 200 }, 5)
    eq(c.owner, nil, "owner")
    eq(c.sphere, true, "sphere")
    c.radius = 7
    near(c.radius, 7, "radius")
    c.world = 4
    eq(c.world, 4, "world")
    c.color = { 0, 255, 0 }
    local colour = c.color
    eq(colour[2], 255, "green")
    eq(colour[4], 200, "alpha kept")
    c.alpha = 100
    eq(c.alpha, 100, "alpha")
    c.position = { -1001, 1001, 11 }
    vec(c.position, -1001, 1001, 11, "position")
    eq(c:destroy(), true, "destroy")
end)

check("Blip", function()
    local b = Blip.create(0, { -1000, 1000, 10 }, 2, 0xFF0000FF, 5)
    local info = b:getInfo()
    eq(info.sprite, 5, "sprite")
    eq(info.scale, 2, "scale")
    vec(info.position, -1000, 1000, 10, "position")
    eq(Blip.destroy(b), true, "destroy")
    eq(b.valid, false, "valid after destroy")
end)

check("Bind", function()
    local k = Bind.create(false, 0x42, 0x43)
    local data = k:getData()
    eq(data.keyOne, 0x42, "keyOne")
    eq(data.keyTwo, 0x43, "keyTwo")
    eq(data.signalsOnRelease, false, "signalsOnRelease")
    k.tag = "test"
    eq(Bind.findByTag("test"), k, "findByTag")
    Bind.create(true, 0x44)
    Bind.clearAllBinds()
    eq(Bind.count(), 0, "clearAllBinds")
    raises("bind no longer exists", function() return k:getData() end)
end)

check("Stream", function()
    local s = Stream()
    s:writeNumber(42)
    s:writeString("hi")
    eq(s:readNumber(), 42, "readNumber")
    eq(s:readString(), "hi", "readString")
    eq(s:send(), true, "send to nobody")
end)

check("Logger and removed globals", function()
    Logger.info("Logger.info works")
    Logger.setLevel("debug")
    eq(Logger.getLevel(), "debug", "level")
    raises("MySQL was removed in v2", function() return MySQL.x end)
end)

-- Leftovers for the reload: run 1 creates entities and a bind that the
-- reload must delete; run 2 checks they are gone.
if not reloaded then
    leftover_vehicle = Vehicle.create(191, 0, -1000, 1000, 10, 0)
    leftover_bind = Bind.create(false, 0x45)
    print("bindings: leftovers " .. leftover_vehicle.id .. " " .. leftover_bind.id)
else
    check("a reload deleted the old runtime's entities and binds", function()
        eq(Vehicle.count(), 0, "vehicles")
        eq(Bind.count(), 0, "binds")
    end)
end

print(("bindings: %d checks, %d failed"):format(checks, #failures))
if #failures > 0 then
    Server.setGame("bindings: failed")
    print("BINDINGS FAIL")
    Server.shutdown()
elseif not reloaded then
    Server.setGame("bindings: reloaded")
    Timer.create(function() Server.reload() end, 100, 1)
else
    print("BINDINGS PASS")
    Timer.create(function() Server.shutdown() end, 100, 1)
end
