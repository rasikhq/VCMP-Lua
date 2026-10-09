-- Phase 3 with a real VC:MP client: the player-facing bindings and events,
-- which the fake server and the player-less run (tests/integration/bindings)
-- cannot cover. See README.md in this directory.
--
-- Every event is logged to the server console. On the first spawn the
-- script checks the Player members automatically and reports PASS/FAIL in
-- the chat and the console; the commands in /help cover the rest by hand.

local results = { pass = 0, fail = 0 }

local function report(player, ok, name, detail)
    local line = ("%s %s%s"):format(ok and "PASS" or "FAIL", name, detail and (": " .. detail) or "")
    print("[client] " .. line)
    if player and player.valid then
        player:msg(line, ok and 0x00FF00FF or 0xFF0000FF)
    end
    results[ok and "pass" or "fail"] = results[ok and "pass" or "fail"] + 1
end

-- check(player, name, fn): fn raises or returns false/message on failure.
local function check(player, name, fn)
    local ok, err = pcall(fn)
    if ok and err ~= false and type(err) ~= "string" then
        report(player, true, name)
    else
        report(player, false, name, tostring(err))
    end
end

local function near(a, b) return type(a) == "number" and math.abs(a - b) < 0.05 end

local function expect(actual, expected, what)
    if actual ~= expected then
        error(("%s: expected %s, got %s"):format(what, tostring(expected), tostring(actual)), 0)
    end
end

-- --- Event log ------------------------------------------------------------------

local function describe(v)
    if type(v) == "table" then
        local parts = {}
        for i, x in ipairs(v) do parts[i] = tostring(x) end
        return "{" .. table.concat(parts, ",") .. "}"
    end
    return tostring(v)
end

local quiet = { onServerFrame = true, onPlayerUpdate = true, onVehicleUpdate = true,
                onEntityStreamingChange = true, onPlayerGameKeysChange = true,
                onServerPerformanceReport = true }
for _, name in ipairs({
    "onServerInit", "onServerShutdown", "onPluginCommand", "onClientData", "onPlayerModuleList",
    "onPlayerConnection", "onPlayerConnect", "onPlayerDisconnect", "onPlayerRequestClass",
    "onPlayerRequestSpawn", "onPlayerSpawn", "onPlayerWasted", "onPlayerKill", "onPlayerUpdate",
    "onPlayerRequestEnterVehicle", "onPlayerEnterVehicle", "onPlayerExitVehicle",
    "onPlayerNameChange", "onPlayerStateChange", "onPlayerActionChange", "onPlayerFireChange",
    "onPlayerCrouchChange", "onPlayerGameKeysChange", "onPlayerBeginTyping",
    "onPlayerFinishTyping", "onPlayerAwayChange", "onPlayerMessage", "onPlayerCommand",
    "onPlayerPM", "onPlayerKeyDown", "onPlayerKeyUp", "onPlayerSpectate", "onPlayerCrashReport",
    "onVehicleUpdate", "onVehicleExplode", "onVehicleRespawn", "onObjectShot", "onObjectTouch",
    "onPickupPickAttempt", "onPickupPicked", "onPickupRespawn", "onCheckpointEnter",
    "onCheckpointExit", "onEntityPoolChange", "onServerPerformanceReport",
    "onEntityStreamingChange",
}) do
    if not quiet[name] then
        Event.bind(name, function(...)
            local args = table.pack(...)
            for i = 1, args.n do args[i] = describe(args[i]) end
            print(("[event] %s(%s)"):format(name, table.concat(args, ", ", 1, args.n)))
        end)
    end
end

-- --- Automatic checks on the first spawn -----------------------------------------

local function automatic(p)
    p:msg("VCMP-Lua phase 3 client test: automatic checks", 0xFFFF00FF)
    check(p, "name, ip, uid, uid2, key", function()
        print(("[client] name=%s ip=%s uid=%s uid2=%s key=%d"):format(p.name, p.ip, p.uid, p.uid2, p.key))
        return #p.name > 0 and #p.ip > 0 and #p.uid > 0
    end)
    check(p, "online, spawned, state", function()
        expect(p.online, true, "online")
        expect(p.spawned, true, "spawned")
        expect(p.state, PlayerState.normal, "state")
    end)
    check(p, "ping, fps, network statistics", function()
        -- GetNetworkStatistics is plugin API 2.1: older servers lack it, and
        -- the binding then raises "not supported by this server version".
        local ok, loss = pcall(p.getNetworkStatistics, p, NetworkStatistics.packetLossTotal)
        if not ok and not tostring(loss):find("not supported by this server version", 1, true) then
            error(loss, 0)
        end
        print(("[client] ping=%d fps=%.1f loss=%s"):format(p.ping, p.fps,
            ok and tostring(loss) or "(not supported by this server)"))
        return p.ping >= 0
    end)
    check(p, "health and armour", function()
        p.health = 55
        p.armour = 45
        expect(near(p.health, 55), true, "health")
        expect(near(p.armour, 45), true, "armour")
        p.health = 100
    end)
    check(p, "cash, score, wanted level", function()
        p.cash = 1234
        expect(p.cash, 1234, "cash")
        p:giveMoney(6)
        expect(p.cash, 1240, "giveMoney")
        p.score = 7
        expect(p.score, 7, "score")
        p.wantedLevel = 2
        expect(p.wantedLevel, 2, "wantedLevel")
        p.wantedLevel = 0
    end)
    check(p, "world, team, skin, colour", function()
        p.world = 3
        expect(p.world, 3, "world")
        p.world = 1
        p.team = 5
        expect(p.team, 5, "team")
        p.skin = 1
        expect(p.skin, 1, "skin")
        p.color = 0xFF8800
        print(("[client] color set to 0xFF8800, reads back 0x%X"):format(p.color))
    end)
    check(p, "position and angle", function()
        local pos = p.position
        p.position = { pos[1], pos[2], pos[3] + 2 }
        local now = p.position
        expect(near(now[3], pos[3] + 2), true, "position z")
        p.angle = 1.5
        expect(near(p.angle, 1.5), true, "angle")
    end)
    check(p, "giveWeapon, setWeapon", function()
        p:disarm()
        p:giveWeapon(26, 150)
        return p:setWeapon(26, 150)
    end)
    -- The server reports the weapon only after the client synced it.
    Timer.create(function()
        if not p.valid then return end
        check(p, "weapon and ammo, after the client synced", function()
            expect(p.weapon, 26, "weapon")
            print(("[client] ammo=%d slot=%d weaponAtSlot=%d ammoAtSlot=%d"):format(p.ammo,
                p.weaponSlot, p:getWeaponAtSlot(p.weaponSlot), p:getAmmoAtSlot(p.weaponSlot)))
            return p.ammo > 0
        end)
    end, 2000, 1)
    check(p, "options, admin, immunity, alpha", function()
        p:setOption(PlayerOption.widescreen, true)
        expect(p:getOption(PlayerOption.widescreen), true, "widescreen on")
        p:setOption(PlayerOption.widescreen, false)
        expect(p:getOption(PlayerOption.widescreen), false, "widescreen off")
        p.admin = true
        expect(p.admin, true, "admin")
        p.admin = false
        p.immunity = 0
        expect(p.immunity, 0, "immunity")
        p:setAlpha(200, 500)
        Timer.create(function() if p.valid then p:setAlpha(255, 500) end end, 1500, 1)
    end)
    check(p, "drunk effects", function()
        p.drunkVisuals = 50
        expect(p.drunkVisuals, 50, "drunkVisuals")
        p.drunkVisuals = 0
        p.drunkHandling = 0
    end)
    check(p, "speed", function()
        p:addSpeed(0, 0, 0.5)
        return type(p.speed) == "table"
    end)
    check(p, "messages, announcements, sounds", function()
        p:msg("chat with a %n and a %s stays literal")
        p:announce("~b~VCMP-Lua ~w~phase 3", 1)
        p:playSound(50)
    end)
    check(p, "Player.findByName and getActive(true)", function()
        expect(Player.findByName(p.name), p, "findByName")
        expect(Player.getActive(true)[p.id], p, "getActive(true)")
    end)
    p:msg(("automatic checks: %d passed, %d failed. Type /help for the rest."):format(
        results.pass, results.fail), results.fail == 0 and 0x00FF00FF or 0xFF0000FF)
    print(("[client] automatic checks: %d passed, %d failed"):format(results.pass, results.fail))
end

-- --- Commands for the rest ----------------------------------------------------------

local made = { vehicles = {} }
local commands = {}

local help = {
    "/car - a vehicle; you are put in it (vehicle, occupant, enter events)",
    "/eject - out of the vehicle (player.vehicle = nil)",
    "/cp - a checkpoint here (walk out and back in)",
    "/pickup - a pickup next to you",
    "/obj - an object next to you that reports shots and touches",
    "/bind - key bind on K: press it (onPlayerKeyDown/Up)",
    "/cam - camera looks at you from above for 3 s",
    "/args a b  c - onPlayerCommand args and text",
    "/name <new> - rename yourself",
    "/stats - pass/fail totals",
    "/reload - Server.reload(); /kick - kick yourself; /kill - kill yourself",
}

commands.help = function(p)
    for _, line in ipairs(help) do p:msg(line, 0xFFFF00FF) end
end

commands.car = function(p)
    local pos = p.position
    local v = Vehicle.create(191, p.world, pos[1] + 3, pos[2], pos[3] + 1, 0)
    made.vehicles[#made.vehicles + 1] = v
    check(p, "setVehicle", function() return p:setVehicle(v) end)
    Timer.create(function()
        if not (p.valid and v.valid) then return end
        check(p, "player.vehicle and getOccupant", function()
            expect(p.vehicle, v, "player.vehicle")
            expect(v:getOccupant(0), p, "getOccupant(0)")
            expect(p.vehicleStatus, PlayerVehicle.inside, "vehicleStatus")
        end)
    end, 1500, 1)
end

commands.eject = function(p)
    p.vehicle = nil
    Timer.create(function()
        if p.valid then check(p, "player.vehicle = nil", function() expect(p.vehicle, nil, "vehicle") end) end
    end, 1500, 1)
end

commands.cp = function(p)
    local c = Checkpoint.create(p, p.world, true, p.position, { 0, 255, 0, 150 }, 3)
    check(p, "checkpoint owner", function() expect(c.owner, p, "owner") end)
end

commands.pickup = function(p)
    local pos = p.position
    Pickup.create(366, p.world, 1, pos[1] + 2, pos[2], pos[3])
    p:msg("walk into the pickup: onPickupPickAttempt / onPickupPicked")
end

commands.obj = function(p)
    local pos = p.position
    local o = Object.create(600, p.world, pos[1] + 3, pos[2], pos[3])
    o.trackShots = true
    o.trackTouch = true
    p:msg("shoot or touch the object: onObjectShot / onObjectTouch")
end

commands.bind = function(p)
    local b = Bind.create(true, 0x4B) -- K
    b.tag = "client-test"
    p:msg("press K (bind " .. b.id .. ")")
end

commands.cam = function(p)
    local pos = p.position
    check(p, "setCamera", function() return p:setCamera({ pos[1], pos[2], pos[3] + 15 }, pos) end)
    Timer.create(function() if p.valid then p:restoreCamera() end end, 3000, 1)
end

commands.args = function(p, args, text)
    p:msg(("args=%s text='%s'"):format(describe(args), text))
end

commands.name = function(p, args)
    local old = p.name
    check(p, "rename", function()
        p.name = args[1]
        expect(p.name, args[1], "name")
    end)
    print("[client] renamed " .. old .. " to " .. p.name)
end

commands.stats = function(p)
    p:msg(("%d passed, %d failed"):format(results.pass, results.fail))
end

commands.reload = function(p) p:msg("reloading the scripts"); Server.reload() end
commands.kick = function(p) p:kick() end
commands.kill = function(p) p:kill() end

Event.bind("onPlayerCommand", function(p, command, args, text)
    local fn = command and commands[command:lower()]
    if fn then
        local ok, err = pcall(fn, p, args or {}, text)
        if not ok then report(p, false, "/" .. command, err) end
    elseif command then
        p:msg("unknown command; /help", 0xFF0000FF)
    end
end)

Event.bind("onPlayerKeyDown", function(p, bind)
    if bind.tag == "client-test" then report(p, true, "onPlayerKeyDown on bind " .. bind.id) end
end)
Event.bind("onCheckpointEnter", function(c, p) report(p, true, "onCheckpointEnter") end)
Event.bind("onPickupPicked", function(k, p) report(p, true, "onPickupPicked") end)
Event.bind("onObjectTouch", function(o, p) report(p, true, "onObjectTouch") end)
Event.bind("onObjectShot", function(o, p, weapon) report(p, true, "onObjectShot with " .. Weapon.getName(weapon)) end)
Event.bind("onPlayerEnterVehicle", function(p, v, slot) report(p, true, "onPlayerEnterVehicle seat " .. slot) end)
Event.bind("onPlayerDisconnect", function(p, reason)
    print(("[client] %s left (reason %d); %d passed, %d failed"):format(p.name, reason, results.pass, results.fail))
end)

local checked = {}
Event.bind("onPlayerSpawn", function(p)
    if not checked[p] then
        checked[p] = true
        Timer.create(function() if p.valid then automatic(p) end end, 1000, 1)
    end
end)

Server.addClass(0, 0xFF8800, 0, { -657.0, 762.0, 11.6, 0.0 }, 26, 500)
Server.addClass(1, 0x0088FF, 1, { -657.0, 762.0, 11.6, 0.0 }, 19, 50)
Server.setClassPosition(-657.0, 762.0, 11.6)
Server.setClassCameraPosition(-653.0, 762.0, 13.0)
Server.setClassCameraLook(-657.0, 762.0, 11.6)
Server.gamemode = "VCMP-Lua P3 test"
print("[client] ready: join the server; /help after spawning")
