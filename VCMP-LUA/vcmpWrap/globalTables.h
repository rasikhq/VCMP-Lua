#pragma once
#include "Log/Logger.h"

#include "EventManager/EventManager.h"
#include "Timer/TimerManager.h"

#include "Classes/Stream.h"
#include "Classes/Server.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"
#include "Classes/Object.h"

void InitGlobals(sol::state*);

void RegisterClasses(sol::state* Lua) {
	sol::state& state = *Lua;

	Logger::Init(Lua, spdlog::level::trace); // Set level to info to avoid debug messages
	InitGlobals(Lua);

	EventManager::Init(Lua);
	TimerManager::Init(Lua);
	Stream::Init(Lua);

	Server::Init(Lua);
	Player::Init(Lua);
	Vehicle::Init(Lua);
	Object::Init(Lua);

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

void InitGlobals(sol::state* Lua) {
	Lua->new_enum("DisconnectReason",
		"timeout",				vcmpDisconnectReason::vcmpDisconnectReasonTimeout,
		"quit",					vcmpDisconnectReason::vcmpDisconnectReasonQuit,
		//
		"kick",					vcmpDisconnectReason::vcmpDisconnectReasonKick,
		"ban",					vcmpDisconnectReason::vcmpDisconnectReasonKick,
		"kickBan",				vcmpDisconnectReason::vcmpDisconnectReasonKick,
		//
		"crash",				vcmpDisconnectReason::vcmpDisconnectReasonCrash,
		"ac",					vcmpDisconnectReason::vcmpDisconnectReasonAntiCheat
	);

	Lua->new_enum("PlayerOption",
		"controllable",			vcmpPlayerOption::vcmpPlayerOptionControllable,
		"driveBy",				vcmpPlayerOption::vcmpPlayerOptionDriveBy,
		"whiteScanLines",		vcmpPlayerOption::vcmpPlayerOptionWhiteScanlines,
		"greenScanLines",		vcmpPlayerOption::vcmpPlayerOptionGreenScanlines,
		"widescreen",			vcmpPlayerOption::vcmpPlayerOptionWidescreen,
		"showMarkers",			vcmpPlayerOption::vcmpPlayerOptionShowMarkers,
		"canAttack",			vcmpPlayerOption::vcmpPlayerOptionCanAttack,
		//
		"hasMarker",			vcmpPlayerOption::vcmpPlayerOptionHasMarker,
		"showOnRadar",			vcmpPlayerOption::vcmpPlayerOptionHasMarker,
		//
		"chatTags",				vcmpPlayerOption::vcmpPlayerOptionChatTagsEnabled,
		"drunkEffects",			vcmpPlayerOption::vcmpPlayerOptionDrunkEffects
	);

	Lua->new_enum("VehicleOption",
		"lockDoors",			vcmpVehicleOption::vcmpVehicleOptionDoorsLocked,
		"alarm",				vcmpVehicleOption::vcmpVehicleOptionAlarm,
		"lights",				vcmpVehicleOption::vcmpVehicleOptionLights,
		"radioLocked",			vcmpVehicleOption::vcmpVehicleOptionRadioLocked,
		"ghost",				vcmpVehicleOption::vcmpVehicleOptionGhost,
		"siren",				vcmpVehicleOption::vcmpVehicleOptionSiren,
		"singleUse",			vcmpVehicleOption::vcmpVehicleOptionSingleUse
	);

	Lua->new_enum("VehicleSpeed",
		"normal",				vcmpVehicleSpeed::Normal,
		"normalRelative",		vcmpVehicleSpeed::NormalRelative,
		"turn",					vcmpVehicleSpeed::Turn,
		"turnRelative",			vcmpVehicleSpeed::TurnRelative
	);
}