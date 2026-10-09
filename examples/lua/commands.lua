-- Chat commands: /pos, /car <model>, /heal and /pm <player> <text>.

local commands = {}

function commands.pos(player)
  local pos = player.position
  player:msg(("x = %.2f, y = %.2f, z = %.2f, world %d"):format(pos[1], pos[2], pos[3], player.world))
end

-- Vehicles live until :destroy(), so remember each player's last one.
function commands.car(player, args)
  local model = tonumber(args and args[1])
  if not model then
    return player:msg("usage: /car <model>")
  end
  local old = player.data.car
  if old and old.valid then
    old:destroy()
  end
  local pos = player.position
  local ok, vehicle = pcall(Vehicle.create, model, player.world, pos[1] + 3, pos[2], pos[3],
    player.angle)
  if not ok then
    return player:msg("cannot create vehicle " .. model)
  end
  player.data.car = vehicle
  player:msg(("spawned vehicle %d (id %d)"):format(model, vehicle.id))
end

function commands.heal(player)
  player.health = 100
end

function commands.pm(player, args, text)
  local target = args and (Player.findByName(args[1]) or Player.findByID(tonumber(args[1]) or -1))
  local message = text and text:match("^%S+%s+(.+)$")
  if not target or not message then
    return player:msg("usage: /pm <name or id> <text>")
  end
  target:msg(("PM from %s: %s"):format(player.name, message), 0xFFFF00FF)
end

Event.bind("onPlayerCommand", function(player, command, args, text)
  local handler = command and commands[command:lower()]
  if handler then
    handler(player, args, text)
  else
    player:msg("unknown command: /" .. tostring(command))
  end
end)

-- Delete a player's car when they leave.
Event.bind("onPlayerDisconnect", function(player)
  local car = player.data.car
  if car and car.valid then
    car:destroy()
  end
end)

-- A message every five minutes, for as long as the server runs.
Timer.create(function()
  Player.msgAll("Type /pos, /car <model>, /heal or /pm <player> <text>.")
end, 5 * 60 * 1000, -1)
