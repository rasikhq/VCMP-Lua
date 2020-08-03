#pragma once
#include "TimerManager.h"
#include "Classes/Server.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

void RegisterClasses(sol::state* Lua) {
	sol::state& state = *Lua;

	TimerManager::Init(Lua);
	Server::Init(Lua);
	Player::Init(Lua);
	Vehicle::Init(Lua);

	Lua->script(R"(
		function INTERNAL__tostring(x, intend)
		intend = tonumber(intend) or 1
		local s
		if type(x) == "table" then
			s = "{\n"
			local i, v = next(x)
			while i do
				for ii = 1, intend do
					s = s .. "\t"
				end
				s = s .. tostring(i) .. " = " .. tostring(v)
				if type(v) == "table" then
					s = s .. " " .. INTERNAL__tostring(v, intend+1)
				end
				i, v = next(x, i)
				if i then s = s .. ",\n" end
			end
			s = s .. "\n"
			for ii = 1, intend-1 do
				s = s .. "\t"
			end
			return s .. "}"
		else 
			return tostring(x)
		end
	  end

	function iprint(...)
		local arg = {...}
		for i = 1, #arg do 
			arg[i] = INTERNAL__tostring(arg[i]) 
			print(arg[i]..(i ~= #arg and ',' or ''))
		end
	end
	)");
}