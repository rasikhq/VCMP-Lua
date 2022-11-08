#pragma once
#include "Log/Logger.h"

#include "Constants.h"
#include "EventManager/EventManager.h"
#include "Timer/TimerManager.h"

#include "Classes/Stream.h"
#include "Classes/Bind.h"
#include "Classes/Server.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"
#include "Classes/Object.h"
#include "Classes/Checkpoint.h"
#include "Classes/Pickup.h"

#include "Modules/MariaDB/MySQL.h"
#include "Modules/Crypto/Hash.h"
#include "Modules/SqLite3/SqLite.h"
#include "Modules/CPR/Remote.h"
#include "Modules/Thread/Thread.h"

// Script modules
#include "Modules/JSON/JSON.hpp"
#include "Modules/Misc/Inspect.hpp"
#include "Modules/Misc/Debugger.hpp"

void InitGlobals(sol::state*);

void RegisterClasses(sol::state* Lua) {
	sol::state& state = *Lua;

	InitGlobals(Lua);

	EventManager::Init(Lua);
	TimerManager::Init(Lua);
	Stream::Init(Lua);
	Bind::Init(Lua);

	// Modules
	MySQL::Init(Lua);
	Hash::Init(Lua);
	SqLite::Init(Lua);
	Remote::Init(Lua);
	Thread::Init(Lua);

	// Wrap
	Server::Init(Lua);
	Player::Init(Lua);
	Vehicle::Init(Lua);
	Object::Init(Lua);
	Checkpoint::Init(Lua);
	Pickup::Init(Lua);

	// Lua Script-Modules
	// JSON
	LoadScriptModule_JSON(Lua);
	LoadScriptModule_Inspect(Lua);
	dbg_setup(Lua->lua_state(), "debugger", "dbg", NULL, NULL);

	/*Lua->script(R"(
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
				local idx = ''
				if type(i) == "string" then idx = '["'..i..'"]'
				else idx = '['..tostring(i)..']' end
				s = s .. idx .. " = " .. "["..type(v).."] "..tostring(v)
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
	)");*/
}

void InitGlobals(sol::state* Lua) {
	/*** GENERIC CONSTANTS ***/
	Lua->new_enum("ServerOption",
		"deathMessages",		vcmpServerOption::vcmpServerOptionDeathMessages,
		"driveBy",				vcmpServerOption::vcmpServerOptionDisableDriveBy,
		"driveOnWater",			vcmpServerOption::vcmpServerOptionDriveOnWater,
		"fastSwitch",			vcmpServerOption::vcmpServerOptionFastSwitch,
		"flyingCars",			vcmpServerOption::vcmpServerOptionFlyingCars,
		"frameLimit",			vcmpServerOption::vcmpServerOptionFrameLimiter,
		"friendlyFire",			vcmpServerOption::vcmpServerOptionFriendlyFire,
		"joinMessages",			vcmpServerOption::vcmpServerOptionJoinMessages,
		"jumpSwitch",			vcmpServerOption::vcmpServerOptionJumpSwitch,
		"shootInAir",			vcmpServerOption::vcmpServerOptionShootInAir,
		"nametags",				vcmpServerOption::vcmpServerOptionShowNameTags,
		"teamMarkersOnly",		vcmpServerOption::vcmpServerOptionOnlyShowTeamMarkers,
		"stuntBike",			vcmpServerOption::vcmpServerOptionStuntBike,
		"syncFrameLimiter",		vcmpServerOption::vcmpServerOptionSyncFrameLimiter,
		"taxiBoostJump",		vcmpServerOption::vcmpServerOptionTaxiBoostJump,
		"wallGlitch",			vcmpServerOption::vcmpServerOptionWallGlitch,
		"deathMessages",		vcmpServerOption::vcmpServerOptionDeathMessages,
		"classes",				vcmpServerOption::vcmpServerOptionUseClasses,
		"chatTags",				vcmpServerOption::vcmpServerOptionChatTagsEnabled,
		"backfaceCulling",		vcmpServerOption::vcmpServerOptionDisableBackfaceCulling,
		"heliBladeDamage",		vcmpServerOption::vcmpServerOptionDisableHeliBladeDamage,
		"perfectHandling",		vcmpServerOption::vcmpServerOptionPerfectHandling,
		"showMarkers",			vcmpServerOption::vcmpServerOptionShowMarkers
	);

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

	Lua->new_enum("BodyPart",
		"body",					vcmpBodyPart::vcmpBodyPartBody,
		"torso",				vcmpBodyPart::vcmpBodyPartTorso,
		"leftArm",				vcmpBodyPart::vcmpBodyPartLeftArm,
		"rightArm",				vcmpBodyPart::vcmpBodyPartRightArm,
		"leftLeg",				vcmpBodyPart::vcmpBodyPartLeftLeg,
		"rightLeg",				vcmpBodyPart::vcmpBodyPartRightArm,
		"head",					vcmpBodyPart::vcmpBodyPartHead,
		"inVehicle",			vcmpBodyPart::vcmpBodyPartInVehicle
	);

	/*** PLAYER CONSTANTS ***/
	Lua->new_enum("PlayerState",
		"none",					vcmpPlayerState::vcmpPlayerStateNone,
		"normal",				vcmpPlayerState::vcmpPlayerStateNormal,
		"aim",					vcmpPlayerState::vcmpPlayerStateAim,
		"driver",				vcmpPlayerState::vcmpPlayerStateDriver,
		"passenger",			vcmpPlayerState::vcmpPlayerStatePassenger,
		"enterDriver",			vcmpPlayerState::vcmpPlayerStateEnterDriver,
		"enterPassenger",		vcmpPlayerState::vcmpPlayerStateEnterPassenger,
		"exit",					vcmpPlayerState::vcmpPlayerStateExit,
		"unspawned",			vcmpPlayerState::vcmpPlayerStateUnspawned
	);

	Lua->new_enum("PlayerUpdate",
		"normal",				vcmpPlayerUpdate::vcmpPlayerUpdateNormal,
		"aiming",				vcmpPlayerUpdate::vcmpPlayerUpdateAiming,
		"driver",				vcmpPlayerUpdate::vcmpPlayerUpdateDriver,
		"passenger",			vcmpPlayerUpdate::vcmpPlayerUpdatePassenger
	);

	Lua->new_enum("PlayerOption",
		"controllable",				vcmpPlayerOption::vcmpPlayerOptionControllable,
		"driveBy",					vcmpPlayerOption::vcmpPlayerOptionDriveBy,
		"whiteScanLines",			vcmpPlayerOption::vcmpPlayerOptionWhiteScanlines,
		"greenScanLines",			vcmpPlayerOption::vcmpPlayerOptionGreenScanlines,
		"widescreen",				vcmpPlayerOption::vcmpPlayerOptionWidescreen,
		"showMarkers",				vcmpPlayerOption::vcmpPlayerOptionShowMarkers,
		"canAttack",				vcmpPlayerOption::vcmpPlayerOptionCanAttack,
		//
		"hasMarker",				vcmpPlayerOption::vcmpPlayerOptionHasMarker,
		"showOnRadar",				vcmpPlayerOption::vcmpPlayerOptionHasMarker,
		//
		"chatTags",					vcmpPlayerOption::vcmpPlayerOptionChatTagsEnabled,
		"drunkEffects",				vcmpPlayerOption::vcmpPlayerOptionDrunkEffects
	);

	Lua->new_enum("PlayerVehicle",
		"outside", vcmpPlayerVehicle::vcmpPlayerVehicleOut,
		"entering", vcmpPlayerVehicle::vcmpPlayerVehicleEntering,
		"exiting", vcmpPlayerVehicle::vcmpPlayerVehicleExiting,
		"inside", vcmpPlayerVehicle::vcmpPlayerVehicleIn
	);

	/*** VEHICLE CONSTANTS ***/
	Lua->new_enum("VehicleUpdate",
		"driverSync",				vcmpVehicleUpdate::vcmpVehicleUpdateDriverSync,
		"otherSync",				vcmpVehicleUpdate::vcmpVehicleUpdateOtherSync,
		"position",					vcmpVehicleUpdate::vcmpVehicleUpdatePosition,
		"health",					vcmpVehicleUpdate::vcmpVehicleUpdateHealth,
		//
		"color",					vcmpVehicleUpdate::vcmpVehicleUpdateColour,
		"colour",					vcmpVehicleUpdate::vcmpVehicleUpdateColour,
		//
		"rotation",					vcmpVehicleUpdate::vcmpVehicleUpdateRotation
	);

	Lua->new_enum("VehicleOption",
		"lockDoors",				vcmpVehicleOption::vcmpVehicleOptionDoorsLocked,
		"alarm",					vcmpVehicleOption::vcmpVehicleOptionAlarm,
		"lights",					vcmpVehicleOption::vcmpVehicleOptionLights,
		"radioLocked",				vcmpVehicleOption::vcmpVehicleOptionRadioLocked,
		"ghost",					vcmpVehicleOption::vcmpVehicleOptionGhost,
		"siren",					vcmpVehicleOption::vcmpVehicleOptionSiren,
		"singleUse",				vcmpVehicleOption::vcmpVehicleOptionSingleUse
	);

	Lua->new_enum("VehicleSpeed",
		"normal",					vcmpVehicleSpeed::Normal,
		"normalRelative",			vcmpVehicleSpeed::NormalRelative,
		"turn",						vcmpVehicleSpeed::Turn,
		"turnRelative",				vcmpVehicleSpeed::TurnRelative
	);

	/*** PICKUP CONSTANTS ***/
	Lua->new_enum("PickupOption",
		"singleUse",				vcmpPickupOption::vcmpPickupOptionSingleUse,
		"forceSize",				vcmpPickupOption::forceSizeVcmpPickupOption
	);

	/*** MISC CONSTANTS ***/
	Lua->new_enum("EntityType",
		"vehicle",					vcmpEntityPool::vcmpEntityPoolVehicle,
		"object",					vcmpEntityPool::vcmpEntityPoolObject,
		"pickup",					vcmpEntityPool::vcmpEntityPoolPickup,
		"radio",					vcmpEntityPool::vcmpEntityPoolRadio,
		"blip",						vcmpEntityPool::vcmpEntityPoolBlip,
		"checkpoint",				vcmpEntityPool::vcmpEntityPoolCheckPoint
	);
}