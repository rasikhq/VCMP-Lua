-- Checkpoint (bindings/checkpoint.cpp) against the recording fake server.

local p = fake.connect("Alice")

test("create: numbers, tables, nil owner, Checkpoint.new and Checkpoint(...)", function()
    local a = Checkpoint.create(p, 0, true, 1, 2, 3, { 255, 0, 0 }, 5)
    expect_call("CreateCheckPoint(0, 0, 1, 1, 2, 3, 255, 0, 0, 255, 5)")
    local b = Checkpoint.create(nil, 1, false, { 1, 2, 3 }, { 0, 255, 0, 128 }, 2.5)
    expect_call("CreateCheckPoint(-1, 1, 0, 1, 2, 3, 0, 255, 0, 128, 2.5)")
    local c = Checkpoint.new(nil, 0, true, { 1, 2, 3 }, { 1, 2, 3 }, 1)
    local d = Checkpoint(p, 0, true, { 1, 2, 3 }, { 1, 2, 3 }, 1)
    expect_call("CreateCheckPoint(0, 0, 1, 1, 2, 3, 1, 2, 3, 255, 1)")
    expect_eq(Checkpoint.count(), 4, "count")
    expect_error("bad argument #5 to 'create' (element 4: value 300 out of range [0, 255])",
        function() Checkpoint.create(nil, 0, true, { 1, 2, 3 }, { 1, 2, 3, 300 }, 1) end)
    expect_error("bad argument #1 to 'create' (Player expected, got number)",
        function() Checkpoint.create(0, 0, true, { 1, 2, 3 }, { 1, 2, 3 }, 1) end)
    expect_error("bad argument #5 to 'create' (table expected, got number)",
        function() Checkpoint.create(nil, 0, true, { 1, 2, 3 }, 255, 1) end)
    for _, x in ipairs({ a, b, c, d }) do x:destroy() end
    expect_eq(Checkpoint.count(), 0, "count after destroy")
end)

local cp = Checkpoint.create(p, 0, true, 0, 0, 0, { 1, 2, 3, 4 }, 5)
local id = cp.id

test("radius is set (v1 compared it with the world first)", function()
    fake.ret("GetCheckPointWorld", 7)
    cp.radius = 7
    expect_call("SetCheckPointRadius(" .. id .. ", 7)")
    fake.ret("GetCheckPointRadius", 3.5)
    expect_eq(cp.radius, 3.5, "radius")
end)

test("colour and alpha", function()
    fake.out("GetCheckPointColour", { 10, 20, 30, 40 })
    local colour = cp.color
    expect_eq(table.concat(colour, ","), "10,20,30,40", "color")
    cp.color = { 1, 2, 3 }
    expect_call("SetCheckPointColour(" .. id .. ", 1, 2, 3, 40)")
    cp.color = { 1, 2, 3, 4 }
    expect_call("SetCheckPointColour(" .. id .. ", 1, 2, 3, 4)")
    expect_eq(cp.alpha, 40, "alpha")
    cp.alpha = 99
    expect_call("SetCheckPointColour(" .. id .. ", 10, 20, 30, 99)")
    expect_error("bad value for 'color' (element 2: value -1 out of range [0, 255])",
        function() cp.color = { 1, -1, 3 } end)
end)

test("members", function()
    cp.world = 2
    expect_call("SetCheckPointWorld(" .. id .. ", 2)")
    cp.position = { 1, 2, 3 }
    expect_call("SetCheckPointPosition(" .. id .. ", 1, 2, 3)")
    fake.out("GetCheckPointPosition", { 4, 5, 6 })
    expect_vec(cp.position, 4, 5, 6, "position")
    fake.ret("GetCheckPointOwner", 0)
    expect_eq(cp.owner, p, "owner")
    expect_eq(cp:getOwner(), p, "getOwner")
    fake.ret("GetCheckPointOwner", -1)
    expect_eq(cp.owner, nil, "no owner")
    fake.ret("IsCheckPointSphere", 1)
    expect_eq(cp.sphere, true, "sphere")
    expect_eq(cp:isSphere(), true, "isSphere")
    fake.ret("IsCheckPointStreamedForPlayer", 1)
    expect_eq(cp:streamedForPlayer(p), true, "streamedForPlayer")
end)

test("lifetime", function()
    collectgarbage()
    expect_eq(fake.exists(EntityType.checkpoint, id), true, "a handle is not the owner")
    cp:destroy()
    expect_call("DeleteCheckPoint(" .. id .. ")")
    expect_error("checkpoint no longer exists", function() return cp.radius end)
end)
