-- Pickup (bindings/pickup.cpp) against the recording fake server.

local p = fake.connect("Alice")

test("create: numbers, a table, Pickup.new and Pickup(...)", function()
    local a = Pickup.create(366, 0, 1, 1, 2, 3)
    expect_call("CreatePickup(366, 0, 1, 1, 2, 3, 255, 1)")
    local b = Pickup.create(366, 0, 50, { 1, 2, 3 }, 128, false)
    expect_call("CreatePickup(366, 0, 50, 1, 2, 3, 128, 0)")
    local c = Pickup.new(367, 1, 1, 1, 2, 3, 255, true)
    expect_call("CreatePickup(367, 1, 1, 1, 2, 3, 255, 1)")
    local d = Pickup(368, 0, 1, { 4, 5, 6 })
    expect_call("CreatePickup(368, 0, 1, 4, 5, 6, 255, 1)")
    expect_eq(Pickup.count(), 4, "count")
    expect_error("bad argument #8 to 'create' (boolean expected, got number)",
        function() Pickup.create(366, 0, 1, 1, 2, 3, 255, 1) end)
    for _, x in ipairs({ a, b, c, d }) do x:destroy() end
    expect_eq(Pickup.count(), 0, "count after destroy")
end)

local k = Pickup.create(366, 0, 1, 0, 0, 0)
local id = k.id

test("members", function()
    k.world = 3
    expect_call("SetPickupWorld(" .. id .. ", 3)")
    k.alpha = 100
    expect_call("SetPickupAlpha(" .. id .. ", 100)")
    k.auto = false
    expect_call("SetPickupIsAutomatic(" .. id .. ", 0)")
    fake.ret("IsPickupAutomatic", 1)
    expect_eq(k.auto, true, "auto")
    k.autoTimer = 5000
    expect_call("SetPickupAutoTimer(" .. id .. ", 5000)")
    k.position = { 1, 2, 3 }
    expect_call("SetPickupPosition(" .. id .. ", 1, 2, 3)")
    fake.out("GetPickupPosition", { 4, 5, 6 })
    expect_vec(k.position, 4, 5, 6, "position")
    fake.ret("GetPickupModel", 366)
    expect_eq(k.model, 366, "model")
    expect_eq(k:getModel(), 366, "getModel")
    fake.ret("GetPickupQuantity", 9)
    expect_eq(k.quantity, 9, "quantity")
    k:setOption(PickupOption.singleUse, true)
    expect_call("SetPickupOption(" .. id .. ", 0, 1)")
    fake.ret("GetPickupOption", 1)
    expect_eq(k:getOption(PickupOption.singleUse), true, "getOption")
    expect_eq(k:respawn(), true, "respawn")
    expect_call("RefreshPickup(" .. id .. ")")
    fake.ret("IsPickupStreamedForPlayer", 0)
    expect_eq(k:streamedForPlayer(p), false, "streamedForPlayer")
end)

test("lifetime", function()
    collectgarbage()
    expect_eq(fake.exists(EntityType.pickup, id), true, "a handle is not the owner")
    k:destroy()
    expect_call("DeletePickup(" .. id .. ")")
    expect_error("pickup no longer exists", function() return k.world end)
end)
