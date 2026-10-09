-- Bind (key binds, bindings/bind.cpp) against the recording fake server.

local p = fake.connect("Alice")

test("create: Bind.create, Bind.new and Bind(...)", function()
    local a = Bind.create(false, 0x42)
    expect_calls({ "GetKeyBindUnusedSlot()", "RegisterKeyBind(0, 0, 66, 0, 0)" })
    expect_eq(a.id, 0, "first slot")
    local b = Bind.new(true, 1, 2, 3)
    expect_call("RegisterKeyBind(1, 1, 1, 2, 3)")
    local c = Bind(false, 1, 2)
    expect_call("RegisterKeyBind(2, 0, 1, 2, 0)")
    expect_eq(Bind.count(), 3, "count")
    expect_eq(Bind.findByID(1), b, "findByID")
    expect_eq(Bind.type(), "Bind", "type")
    expect_error("bad argument #1 to 'create' (boolean expected, got number)",
        function() Bind.create(0, 1) end)
    for _, x in ipairs({ a, b, c }) do x:destroy() end
    expect_eq(fake.binds(), 0, "removed")
end)

test("getData and tag", function()
    local b = Bind.create(true, 10, 20)
    local data = b:getData()
    expect_eq(data.keyOne, 10, "keyOne")
    expect_eq(data.keyTwo, 20, "keyTwo")
    expect_eq(data.keyThree, 0, "keyThree")
    expect_eq(data.signalsOnRelease, true, "signalsOnRelease")
    expect_eq(b.tag, "", "no tag")
    b.tag = "menu"
    expect_eq(b.tag, "menu", "tag")
    expect_eq(Bind.findByTag("menu"), b, "findByTag")
    expect_eq(Bind.findByTag("none"), nil, "findByTag of no bind")
    b.data.page = 2
    expect_eq(Bind.findByTag("menu").data.page, 2, "data")
    b:destroy()
    expect_error("bind no longer exists", function() return b.tag end)
end)

test("a bind removed by someone else is noticed on use", function()
    local b = Bind.create(false, 5)
    fake.unbind(b.id)
    expect_error("bind no longer exists", function() return b:getData() end)
    expect_eq(b.valid, false, "released")
end)

test("clearAllBinds removes only this plugin's binds", function()
    fake.bind(200, false, 1, 0, 0)  -- another plugin's
    Bind.create(false, 1)
    Bind.create(false, 2)
    fake.clear()
    Bind.clearAllBinds()
    for _, call in ipairs(fake.calls()) do
        if call:find("RemoveAllKeyBinds", 1, true) or call:find("RemoveKeyBind(200)", 1, true) then
            error("removed another plugin's bind: " .. call)
        end
    end
    expect_eq(fake.binds(), 1, "the other plugin's bind stays")
    expect_eq(Bind.count(), 0, "ours are gone")
end)

test("no free slot", function()
    for id = 0, 255 do fake.bind(id, false, 1, 0, 0) end
    expect_error("'create' failed: no free key bind slot", function() Bind.create(false, 1) end)
    for id = 0, 255 do fake.unbind(id) end
end)
