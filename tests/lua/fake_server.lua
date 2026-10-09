-- The fake server itself: the recorder and the world the Lua tests run in.

test("fake.connect adopts the player and returns the handle", function()
    local player = fake.connect("Alice")
    expect_eq(player.id, 0, "id")
    expect_eq(fake.connect().id, 1, "second id")
    fake.disconnect(1)
end)

test("entity creation and deletion are recorded", function()
    local vehicle = test_create_vehicle()
    local id = vehicle.id
    expect_calls({ "CreateVehicle(130, 0, 0, 0, 0, 0, -1, -1)" })
    expect_eq(fake.exists(1, id), true, "exists")
    test_delete_vehicle(vehicle)
    expect_call("DeleteVehicle(" .. id .. ")")
    expect_eq(fake.exists(1, id), false, "exists after delete")
end)

test("another plugin's entities are adopted from the pool events", function()
    local id = fake.create(1)
    expect_eq(test_vehicle(id).id, id, "adopted")
    fake.delete(1, id)
    expect_eq(test_vehicle(id), nil, "released")
end)
