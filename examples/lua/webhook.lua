-- Posts joins and leaves to a web service as JSON, without blocking the
-- server: the callback runs in a later server frame.

local cjson = require "cjson"
local http = require "http"

-- Set this to your endpoint, e.g. a Discord webhook URL.
local WEBHOOK_URL = nil

local function post(text)
  if not WEBHOOK_URL then
    return
  end
  http.request({
    url = WEBHOOK_URL,
    method = "POST",
    headers = { ["Content-Type"] = "application/json" },
    body = cjson.encode({ content = text }),
    timeout = 10,
  }, function(res, err)
    if not res then
      Logger.warn("webhook failed: " .. err)
    elseif res.status >= 300 then
      Logger.warn(("webhook returned HTTP %d: %s"):format(res.status, res.body))
    end
  end)
end

Event.bind("onPlayerConnect", function(player)
  post(player.name .. " joined the server")
end)

Event.bind("onPlayerDisconnect", function(player)
  post(player.name .. " left the server")
end)
