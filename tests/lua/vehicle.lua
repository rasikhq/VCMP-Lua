-- Vehicle (bindings/vehicle.cpp) against the recording fake server.

local p = fake.connect("Alice")

test("create: numbers, a table, Vehicle.new and Vehicle(...)", function()
    local a = Vehicle.create(130, 0, 1, 2, 3, 0.5)
    expect_call("CreateVehicle(130, 0, 1, 2, 3, 0.5, -1, -1)")
    expect_eq(a.id, 1, "first vehicle id")
    expect_eq(Vehicle.findByID(1), a, "findByID")
    local b = Vehicle.create(131, 1, { 4, 5, 6, 1.5 }, 2, 3)
    expect_call("CreateVehicle(131, 1, 4, 5, 6, 1.5, 2, 3)")
    local c = Vehicle.new(132, 0, { 7, 8, 9 })
    expect_call("CreateVehicle(132, 0, 7, 8, 9, 0, -1, -1)")
    local d = Vehicle(133, 0, 1, 1, 1, 0, 5)
    expect_call("CreateVehicle(133, 0, 1, 1, 1, 0, 5, -1)")
    expect_eq(Vehicle.count(), 4, "count")
    for _, v in ipairs({ a, b, c, d }) do v:destroy() end
    expect_eq(Vehicle.count(), 0, "count after destroy")
end)

test("create refuses bad arguments and reports the server's error", function()
    expect_error("bad argument #1 to 'create' (integer expected, got string)",
        function() Vehicle.create("infernus", 0, 1, 2, 3, 0) end)
    expect_error("bad argument #6 to 'create' (number expected, got no value)",
        function() Vehicle.create(130, 0, 1, 2, 3) end)
    expect_error("bad argument #3 to 'create' (element 2 must be a number, got nil)",
        function() Vehicle.create(130, 0, { 1 }) end)
    fake.ret("CreateVehicle", -1)
end)

test("the server owns vehicles: collecting the handle deletes nothing", function()
    local id = Vehicle.create(130, 0, 0, 0, 0, 0).id
    collectgarbage()
    collectgarbage()
    expect_eq(fake.exists(EntityType.vehicle, id), true, "still exists")
    local v = Vehicle.findByID(id)
    expect_eq(v:destroy(), true, "destroy")
    expect_call("DeleteVehicle(" .. id .. ")")
    expect_eq(v.valid, false, "valid after destroy")
    expect_error("vehicle no longer exists", function() v:destroy() end)
    expect_eq(fake.exists(EntityType.vehicle, id), false, "deleted once")
end)

test("a vehicle deleted by someone else is noticed", function()
    local v = Vehicle.create(130, 0, 0, 0, 0, 0)
    local id = v.id
    fake.delete(EntityType.vehicle, id)
    expect_error("vehicle no longer exists", function() return v.health end)
    local w = Vehicle.create(130, 0, 0, 0, 0, 0)
    expect_eq(w.id, id, "the id is reused")
    expect_eq(v == w, false, "but the old handle is not the new vehicle")
    w:destroy()
end)

local v = Vehicle.create(130, 0, 0, 0, 0, 0)
local id = v.id

test("properties", function()
    v.health = 500
    expect_call("SetVehicleHealth(" .. id .. ", 500)")
    fake.ret("GetVehicleHealth", 250)
    expect_eq(v.health, 250, "health")
    v.world = 3
    expect_call("SetVehicleWorld(" .. id .. ", 3)")
    v.idleRespawnTime = 60000
    expect_call("SetVehicleIdleRespawnTimer(" .. id .. ", 60000)")
    v.radio = 2
    expect_call("SetVehicleRadio(" .. id .. ", 2)")
    v.damage = 0
    expect_call("SetVehicleDamageData(" .. id .. ", 0)")
    v.immunity = 255
    expect_call("SetVehicleImmunityFlags(" .. id .. ", 255)")
    fake.ret("GetVehicleModel", 191)
    expect_eq(v.model, 191, "model")
    expect_eq(v:getModel(), 191, "getModel")
    fake.ret("IsVehicleWrecked", 1)
    expect_eq(v.wrecked, true, "wrecked")
end)

test("taxiLight is bit 8 of the lights data", function()
    fake.ret("GetVehicleLightsData", 0x0F)
    expect_eq(v.taxiLight, false, "off")
    v.taxiLight = true
    expect_call("SetVehicleLightsData(" .. id .. ", 271)")
    fake.ret("GetVehicleLightsData", 0x10F)
    expect_eq(v.taxiLight, true, "on")
    v.taxiLight = false
    expect_call("SetVehicleLightsData(" .. id .. ", 15)")
end)

test("repair", function()
    fake.ret("GetVehicleLightsData", 0x1FF)
    v:repair()
    expect_calls({ "SetVehicleHealth(" .. id .. ", 1000)", "SetVehicleDamageData(" .. id .. ", 0)",
                   "GetVehicleLightsData(" .. id .. ")", "SetVehicleLightsData(" .. id .. ", 256)" })
    fake.clear()
    v:fix()
    expect_eq(#fake.calls(), 4, "fix is repair")
end)

test("colours", function()
    fake.out("GetVehicleColour", { 3, 7 })
    local colour = v.color
    expect_eq(colour[1], 3, "primary")
    expect_eq(colour[2], 7, "secondary")
    v.color = { 9 }
    expect_call("SetVehicleColour(" .. id .. ", 9, 7)")
    v.color = { 1, 2 }
    expect_call("SetVehicleColour(" .. id .. ", 1, 2)")
    expect_error("bad value for 'color' (table expected, got number)", function() v.color = 5 end)
end)

test("rotation round-trips", function()
    fake.out("GetVehicleRotationEuler", { 0.1, 0.2, 0.3 })
    fake.out("GetVehicleRotation", { 0, 0, 0.5, 0.75 })
    local rotation = v.rotation
    expect_vec(rotation.euler, 0.1, 0.2, 0.3, "euler")
    expect_near(rotation.quaternion[4], 0.75, "quaternion w")
    v.rotation = rotation
    expect_call("SetVehicleRotation(" .. id .. ", 0, 0, 0.5, 0.75)")
    v.rotation = { euler = { 1, 2, 3 } }
    expect_call("SetVehicleRotationEuler(" .. id .. ", 1, 2, 3)")
    v.angle = { 1, 2, 3 }
    expect_call("SetVehicleRotationEuler(" .. id .. ", 1, 2, 3)")
    v:setRotation(1, 2, 3, 4)
    expect_call("SetVehicleRotation(" .. id .. ", 1, 2, 3, 4)")
    v:setRotation(1, 2, 3)
    expect_call("SetVehicleRotationEuler(" .. id .. ", 1, 2, 3)")
    expect_eq(v:getRotation().quaternion[3], 0.5, "getRotation")
    v.spawnRotation = { 1, 2, 3, 4 }
    expect_call("SetVehicleSpawnRotation(" .. id .. ", 1, 2, 3, 4)")
    v.spawnRotation = { 1, 2, 3 }
    expect_call("SetVehicleSpawnRotationEuler(" .. id .. ", 1, 2, 3)")
end)

test("positions", function()
    v.position = { 1, 2, 3 }
    expect_call("SetVehiclePosition(" .. id .. ", 1, 2, 3, 0)")
    fake.out("GetVehiclePosition", { 4, 5, 6 })
    expect_vec(v.position, 4, 5, 6, "position")
    v.spawnPosition = { 7, 8, 9 }
    expect_call("SetVehicleSpawnPosition(" .. id .. ", 7, 8, 9)")
end)

test("speeds", function()
    fake.out("GetVehicleSpeed", { 1, 2, 3 })
    expect_vec(v:getSpeed(), 1, 2, 3, "getSpeed()")
    expect_call("GetVehicleSpeed(" .. id .. ", *, *, *, 0)")
    v:getSpeed(VehicleSpeed.normalRelative)
    expect_call("GetVehicleSpeed(" .. id .. ", *, *, *, 1)")
    v:getSpeed(VehicleSpeed.turnRelative)
    expect_call("GetVehicleTurnSpeed(" .. id .. ", *, *, *, 1)")
    expect_error("VehicleSpeed value expected", function() v:getSpeed(9) end)
    v:setSpeed(1, 2, 3, true)
    expect_call("SetVehicleSpeed(" .. id .. ", 1, 2, 3, 1, 0)")
    v:setSpeed(1, 2, 3)
    expect_call("SetVehicleSpeed(" .. id .. ", 1, 2, 3, 0, 0)")
    v:setSpeed(VehicleSpeed.turn, 1, 2, 3, false)
    expect_call("SetVehicleTurnSpeed(" .. id .. ", 1, 2, 3, 0, 0)")
    v:setSpeed(VehicleSpeed.normalRelative, { 4, 5, 6 }, true)
    expect_call("SetVehicleSpeed(" .. id .. ", 4, 5, 6, 1, 1)")
    v:setSpeed({ 7, 8, 9 })
    expect_call("SetVehicleSpeed(" .. id .. ", 7, 8, 9, 0, 0)")
end)

test("handling rules", function()
    v:setHandlingRule(1, 2.5)
    expect_call("SetInstHandlingRule(" .. id .. ", 1, 2.5)")
    fake.ret("GetInstHandlingRule", 3.25)
    expect_eq(v:getHandlingRule(1), 3.25, "getHandlingRule")
    fake.ret("ExistsInstHandlingRule", 1)
    expect_eq(v:hasHandlingRule(1), true, "hasHandlingRule")
    v:resetHandlingRule(4)
    expect_call("ResetInstHandlingRule(" .. id .. ", 4)")
    fake.clear()
    v:resetHandlingRule({ 5, 6 })
    expect_calls({ "ResetInstHandlingRule(" .. id .. ", 5)", "ResetInstHandlingRule(" .. id .. ", 6)" })
    v:resetHandling()
    expect_call("ResetInstHandling(" .. id .. ")")
    Vehicle.setModelHandlingRule(130, 1, 0.5)
    expect_call("SetHandlingRule(130, 1, 0.5)")
    fake.ret("ExistsHandlingRule", 0)
    expect_eq(Vehicle.modelHandlingRuleExists(130, 1), false, "modelHandlingRuleExists")
    Vehicle.resetModelHandlingRule(130, 1)
    expect_call("ResetHandlingRule(130, 1)")
    Vehicle.resetModelHandlingRules(130)
    expect_call("ResetHandling(130)")
    Vehicle.resetAllHandlings()
    expect_call("ResetAllVehicleHandlings()")
end)

test("occupants, options, parts", function()
    fake.ret("GetVehicleOccupant", 0)
    expect_eq(v:getOccupant(0), p, "driver")
    fake.error("GetVehicleOccupant", 1)
    expect_eq(v:getOccupant(1), nil, "empty seat")
    fake.error("GetVehicleOccupant", 4)
    expect_error("'getOccupant' failed: argument out of bounds", function() v:getOccupant(99) end)
    fake.error("GetVehicleOccupant", 0)
    v:setOption(VehicleOption.siren, true)
    expect_call("SetVehicleOption(" .. id .. ", 5, 1)")
    fake.ret("GetVehicleOption", 1)
    expect_eq(v:getOption(VehicleOption.siren), true, "getOption")
    v:setPartStatus(1, 2)
    expect_call("SetVehiclePartStatus(" .. id .. ", 1, 2)")
    v:setTyreStatus(3, 1)
    expect_call("SetVehicleTyreStatus(" .. id .. ", 3, 1)")
    fake.out("GetVehicleTurretRotation", { 0.25, 0.5 })
    local h, vert = v:getTurretRotation()
    expect_eq(h, 0.25, "horizontal")
    expect_eq(vert, 0.5, "vertical")
    fake.ret("IsVehicleStreamedForPlayer", 1)
    expect_eq(v:streamedForPlayer(p), true, "streamedForPlayer")
    v:set3DArrowToPlayer(p, true)
    expect_call("SetVehicle3DArrowForPlayer(" .. id .. ", 0, 1)")
    v:respawn()
    expect_call("RespawnVehicle(" .. id .. ")")
    v:explode()
    expect_call("ExplodeVehicle(" .. id .. ")")
end)

test("table elements are checked", function()
    expect_error("bad value for 'color' (element 1 has no integer representation)",
        function() v.color = { 1.5 } end)
    expect_error("bad argument #1 to 'resetHandlingRule' (element 2 must be an integer, got string)",
        function() v:resetHandlingRule({ 1, "x" }) end)
end)

test("tables are read without metamethods", function()
    local touched = false
    local sneaky = setmetatable({ 1, 2, 3 }, { __index = function() touched = true end })
    v.rotation = sneaky
    expect_call("SetVehicleRotationEuler(" .. id .. ", 1, 2, 3)")
    expect_eq(touched, false, "__index ran inside the binding")
end)
