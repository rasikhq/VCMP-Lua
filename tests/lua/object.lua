-- Object (bindings/object.cpp) against the recording fake server.

local p = fake.connect("Alice")

test("create: numbers, a table, Object.new and Object(...)", function()
    local a = Object.create(600, 0, 1, 2, 3)
    expect_call("CreateObject(600, 0, 1, 2, 3, 255)")
    expect_eq(a.id, 0, "first object id")
    local b = Object.create(601, 1, 1, 2, 3, 100)
    expect_call("CreateObject(601, 1, 1, 2, 3, 100)")
    local c = Object.new(602, 0, { 4, 5, 6, 50 })
    expect_call("CreateObject(602, 0, 4, 5, 6, 50)")
    local d = Object(603, 0, { 4, 5, 6 })
    expect_call("CreateObject(603, 0, 4, 5, 6, 255)")
    expect_eq(Object.count(), 4, "count")
    expect_error("bad argument #6 to 'create' (value 256 out of range [0, 255])",
        function() Object.create(600, 0, 1, 2, 3, 256) end)
    for _, o in ipairs({ a, b, c, d }) do expect_eq(o:destroy(), true, "destroy") end
    expect_eq(Object.count(), 0, "count after destroy")
end)

local o = Object.create(600, 0, 0, 0, 0)
local id = o.id

test("properties", function()
    o.world = 2
    expect_call("SetObjectWorld(" .. id .. ", 2)")
    o.trackShots = true
    expect_call("SetObjectShotReportEnabled(" .. id .. ", 1)")
    fake.ret("IsObjectTouchedReportEnabled", 1)
    expect_eq(o.trackTouch, true, "trackTouch")
    o.trackTouch = false
    expect_call("SetObjectTouchedReportEnabled(" .. id .. ", 0)")
    fake.ret("GetObjectModel", 600)
    expect_eq(o.model, 600, "model")
    expect_eq(o:getModel(), 600, "getModel")
    o.alpha = 100
    expect_call("SetObjectAlpha(" .. id .. ", 100, 0)")
    fake.ret("GetObjectAlpha", 100)
    expect_eq(o:getAlpha(), 100, "getAlpha")
    o:setAlpha(50, 1000)
    expect_call("SetObjectAlpha(" .. id .. ", 50, 1000)")
    o.position = { 1, 2, 3 }
    expect_call("SetObjectPosition(" .. id .. ", 1, 2, 3)")
    fake.out("GetObjectPosition", { 4, 5, 6 })
    expect_vec(o.position, 4, 5, 6, "position")
end)

test("angle = rotates to the angle, not by it", function()
    o.angle = { 0.1, 0.2, 0.3 }
    expect_call("RotateObjectToEuler(" .. id .. ", 0.1, 0.2, 0.3, 0)")
    fake.out("GetObjectRotationEuler", { 1, 2, 3 })
    expect_vec(o.angle, 1, 2, 3, "angle")
end)

test("moveTo and moveBy", function()
    o:moveTo(1, 2, 3)
    expect_call("MoveObjectTo(" .. id .. ", 1, 2, 3, 0)")
    o:moveTo(1, 2, 3, 500)
    expect_call("MoveObjectTo(" .. id .. ", 1, 2, 3, 500)")
    o:moveTo({ 1, 2, 3, 700 })
    expect_call("MoveObjectTo(" .. id .. ", 1, 2, 3, 700)")
    o:moveTo({ 1, 2, 3 }, 900)
    expect_call("MoveObjectTo(" .. id .. ", 1, 2, 3, 900)")
    o:moveBy(0, 0, 1, 100)
    expect_call("MoveObjectBy(" .. id .. ", 0, 0, 1, 100)")
    o:moveBy({ 0, 0, 2 })
    expect_call("MoveObjectBy(" .. id .. ", 0, 0, 2, 0)")
    expect_error("bad argument #4 to 'moveTo' (value -1 out of range [0, 4294967295])",
        function() o:moveTo(1, 2, 3, -1) end)
end)

test("rotateTo and rotateBy: Euler or quaternion", function()
    o:rotateTo({ 1, 2, 3 })
    expect_call("RotateObjectToEuler(" .. id .. ", 1, 2, 3, 0)")
    o:rotateTo({ 0, 0, 0, -1 }, 200)
    expect_call("RotateObjectTo(" .. id .. ", 0, 0, 0, -1, 200)")
    o:rotateBy({ 1, 2, 3 }, 300)
    expect_call("RotateObjectByEuler(" .. id .. ", 1, 2, 3, 300)")
    o:rotateBy({ 1, 2, 3, 4 })
    expect_call("RotateObjectBy(" .. id .. ", 1, 2, 3, 4, 0)")
    expect_error("bad argument #1 to 'rotateTo' (table expected, got number)",
        function() o:rotateTo(1, 2, 3) end)
end)

test("streaming and lifetime", function()
    fake.ret("IsObjectStreamedForPlayer", 1)
    expect_eq(o:streamedForPlayer(p), true, "streamedForPlayer")
    expect_call("IsObjectStreamedForPlayer(" .. id .. ", 0)")
    collectgarbage()
    expect_eq(fake.exists(EntityType.object, id), true, "a handle is not the owner")
    o:destroy()
    expect_eq(o.valid, false, "valid after destroy")
    expect_error("object no longer exists", function() o:moveTo(1, 2, 3) end)
end)
