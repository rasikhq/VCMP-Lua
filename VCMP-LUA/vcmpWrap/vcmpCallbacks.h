#pragma once
#include "utility.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

#define DEBUG_ENABLED
#undef DEBUG_ENABLED

extern PluginFuncs* g_Funcs;
extern PluginCallbacks* g_Calls;
extern lua_State* Lua;

void RegisterVCMPCallbacks() {
	/*** Server ***/
	g_Calls->OnServerInitialise = [] () -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "onServerinitialise" << std::endl;
#endif
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onServerInit");
			if (fn.isFunction())
				fn();
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onServerInit");
		}
		return (uint8_t) 1;
	};

	g_Calls->OnServerShutdown = []() -> void {
#ifdef DEBUG_ENABLED
		std::cout << "OnServerShutdown" << std::endl;
#endif
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onServerShutdown");
			if (fn.isFunction())
				fn();
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onServerShutdown");
		}
	};
	g_Calls->OnServerFrame = [](float elapsedTime) -> void {
#ifdef DEBUG_ENABLED
		//std::cout << "OnServerFrame" << std::endl;
#endif
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onServerFrame");
			if (fn.isFunction())
				fn(elapsedTime);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onServerFrame");
		}
	};
	
	g_Calls->OnPluginCommand = [](uint32_t commandIdentifier, const char* message) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPluginCommand" << std::endl;
#endif
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPluginCommand");
			if (fn.isFunction())
				fn(commandIdentifier, message);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPluginCommand");
		}
		return (uint8_t) 1;
	};

	g_Calls->OnIncomingConnection = [](char* playerName, size_t nameBufferSize, const char* userPassword, const char* ipAddress) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnIncomingConnection" << std::endl;
#endif
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerConnection");
			if (fn.isFunction())
				fn(playerName, nameBufferSize, userPassword, ipAddress);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onConnectionReceive");
		}
		return (uint8_t) 1;
	};

	/*** Player ***/
	g_Calls->OnClientScriptData = [](int32_t playerId, const uint8_t* data, size_t size) {
		
	};
	
	g_Calls->OnPlayerConnect = [](int32_t playerId) {
#ifdef DEBUG_ENABLED
		std::cout << "onPlayerConnect" << std::endl;
#endif
		Player* player = Player::Register(playerId);
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerJoin");
			if (fn.isFunction())
				fn(*player);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerConnect");
		}
	};
	
	g_Calls->OnPlayerDisconnect = [](int32_t playerId, vcmpDisconnectReason reason) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerDisconnect" << std::endl;
#endif
		Player* player = Player::Get(playerId);
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerDisconnect");
			if (fn.isFunction())
				fn(*player);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerDisconnect");
		}
	};
	g_Calls->OnPlayerModuleList = [](int32_t playerId, const char* list) {
		
	};

	g_Calls->OnPlayerRequestClass = [](int32_t playerId, int32_t offset) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerRequestClass" << std::endl;
#endif
		Player* player = Player::Get(playerId);
		uint8_t ret = 1;
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerRequestClass");
			if (fn.isFunction()) {
				luabridge::LuaRef result = fn(*player);
				if (result.isNumber())
					ret = result.cast<uint8_t>();
			}
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerRequestClass");
		}
		return ret;
	};

	g_Calls->OnPlayerRequestSpawn = [](int32_t playerId) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerRequestSpawn" << std::endl;
#endif
		Player* player = Player::Get(playerId);
		uint8_t ret = 1;
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerRequestSpawn");
			if (fn.isFunction()) {
				luabridge::LuaRef result = fn(*player);
				if (result.isNumber())
					ret = result.cast<uint8_t>();
			}
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerRequestSpawn");
		}
		return ret;
	};

	g_Calls->OnPlayerSpawn = [](int32_t playerId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerSpawn" << std::endl;
#endif
		Player* player = Player::Get(playerId);
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerSpawn");
			if (fn.isFunction())
				fn(*player);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerSpawn");
		}
	};
	g_Calls->OnPlayerDeath = [](int32_t playerId, int32_t killerId, int32_t reason, vcmpBodyPart bodyPart) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerDeath" << std::endl;
#endif
		if (!g_Funcs->IsPlayerConnected(killerId)) {
			try {
				Player* player = Player::Get(playerId);
				luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerWasted");
				if (fn.isFunction()) {
					if (reason == 43 || reason == 50)
						reason = 43; // drowned
					else if (reason == 39 && bodyPart == 7)
						reason = 39; // car crash
					else if (reason == 39 || reason == 40 || reason == 44)
						reason = 44; // fell

					fn(*player, reason);
					return;
				}
			}
			catch (luabridge::LuaException e) {
				OutputErrorCallbackCall("onPlayerWasted");
			}
		}

		try {
			Player* player = Player::Get(playerId);
			Player* killer = Player::Get(killerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerKill");
			if (fn.isFunction())
				fn(*killer, *player, reason, bodyPart);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerKill");
		}
	};
	g_Calls->OnPlayerUpdate = [](int32_t playerId, vcmpPlayerUpdate updateType) {
#ifdef DEBUG_ENABLED
		//std::cout << "OnPlayerUpdate" << std::endl;
#endif
		Player* player = Player::Get(playerId);
		try {
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerUpdate");
			if (fn.isFunction())
				fn(*player);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerUpdate");
		}
	};

	g_Calls->OnPlayerRequestEnterVehicle = [](int32_t playerId, int32_t vehicleId, int32_t slotIndex) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerRequestEnterVehicle" << std::endl;
#endif
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerRequestEnterVehicle");
			if (fn.isFunction()) {
				luabridge::LuaRef result = fn(*player, *vehicle, vehicleId, slotIndex);
				if (result.isNumber())
					ret = result.cast<uint8_t>();
			}
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerRequestEnterVehicle");
		}
		return ret;
	};
	g_Calls->OnPlayerEnterVehicle = [](int32_t playerId, int32_t vehicleId, int32_t slotIndex) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerEnterVehicle" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerEnterVehicle");
			if (fn.isFunction())
				fn(*player, *vehicle, slotIndex);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerEnterVehicle");
		}
	};
	g_Calls->OnPlayerExitVehicle = [](int32_t playerId, int32_t vehicleId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerExitVehicle" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerExitVehicle");
			if (fn.isFunction())
				fn(*player, *vehicle);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerExitVehicle");
		}
	};

	g_Calls->OnPlayerNameChange = [](int32_t playerId, const char* oldName, const char* newName) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerNameChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerNameChange");
			if (fn.isFunction())
				fn(*player, oldName, newName);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerNameChange");
		}
	};
	g_Calls->OnPlayerStateChange = [](int32_t playerId, vcmpPlayerState oldState, vcmpPlayerState newState) {
#ifdef DEBUG_ENABLED
		//std::cout << "OnPlayerStateChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerStateChange");
			if (fn.isFunction())
				fn(*player, oldState, newState);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerStateChange");
		}
	};
	g_Calls->OnPlayerActionChange = [](int32_t playerId, int32_t oldAction, int32_t newAction) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerActionChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerActionChange");
			if (fn.isFunction())
				fn(*player, oldAction, newAction);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerActionChange");
		}
	};
	g_Calls->OnPlayerOnFireChange = [](int32_t playerId, uint8_t isOnFire) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerOnFireChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerFireChange");
			if (fn.isFunction())
				fn(*player, isOnFire);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerFireChange");
		}
	};
	g_Calls->OnPlayerCrouchChange = [](int32_t playerId, uint8_t isCrouching) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerCrouchChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerCrouchChange");
			if (fn.isFunction())
				fn(*player, isCrouching);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerCrouchChange");
	}
	};
	g_Calls->OnPlayerGameKeysChange = [](int32_t playerId, uint32_t oldKeys, uint32_t newKeys) {
#ifdef DEBUG_ENABLED
		//std::cout << "OnPlayerGameKeysChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerGameKeysChange");
			if (fn.isFunction())
				fn(*player, oldKeys, newKeys);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerGameKeysChange");
	}
	};
	g_Calls->OnPlayerBeginTyping = [](int32_t playerId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerBeginTyping" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerBeginTyping");
			if (fn.isFunction())
				fn(*player);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerBeginTyping");
		}
	};
	g_Calls->OnPlayerEndTyping = [](int32_t playerId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerEndTyping" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerFinishTyping");
			if (fn.isFunction())
				fn(*player);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerFinishTyping");
		}
	};
	g_Calls->OnPlayerAwayChange = [](int32_t playerId, uint8_t isAway) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerAwayChange" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerAwayChange");
			if (fn.isFunction())
				fn(*player, isAway);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerAwayChange");
		}
	};

	g_Calls->OnPlayerMessage = [](int32_t playerId, const char* message) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerMessage" << std::endl;
#endif
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerMessage");
			if (fn.isFunction()) {
				luabridge::LuaRef result = fn(*player, message);
				if (result.isNumber())
					ret = result.cast<uint8_t>();
			}
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerMessage");
		}
		return ret;
	};
	g_Calls->OnPlayerCommand = [](int32_t playerId, const char* message) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerCommand" << std::endl;
#endif
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerCommand");
			if (fn.isFunction()) {
				luabridge::LuaRef result = fn(*player, message);
				if (result.isNumber())
					ret = result.cast<uint8_t>();
			}
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerCommand");
	}
		return ret;
	};
	g_Calls->OnPlayerPrivateMessage = [](int32_t playerId, int32_t targetPlayerId, const char* message) -> uint8_t {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerPrivateMessage" << std::endl;
#endif
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			Player* targetPlayer = Player::Get(targetPlayerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerPM");
			if (fn.isFunction()) {
				luabridge::LuaRef result = fn(*player, *targetPlayer, message);
				if (result.isNumber())
					ret = result.cast<uint8_t>();
			}
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerPrivateMessage");
		}
		return ret;
	};

	g_Calls->OnPlayerKeyBindDown = [](int32_t playerId, int32_t bindId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerKeyBindDown" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerKeyDown");
			if (fn.isFunction())
				fn(*player, bindId);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerKeyDown");
	}
	};
	g_Calls->OnPlayerKeyBindUp = [](int32_t playerId, int32_t bindId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerKeyBindUp" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerKeyUp");
			if (fn.isFunction())
				fn(*player, bindId);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerKeyUp");
		}
	};
	g_Calls->OnPlayerSpectate = [](int32_t playerId, int32_t targetPlayerId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerSpectate" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			Player* targetPlayer = Player::Get(targetPlayerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerSpectate");
			if (fn.isFunction())
				fn(*player, *targetPlayer);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerSpectate");
		}
	};
	g_Calls->OnPlayerCrashReport = [](int32_t playerId, const char* report) {
#ifdef DEBUG_ENABLED
		std::cout << "OnPlayerCrashReport" << std::endl;
#endif
		try {
			Player* player = Player::Get(playerId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onPlayerCrashReport");
			if (fn.isFunction())
				fn(*player, report);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onPlayerCrashReport");
		}
	};
	
	
	g_Calls->OnVehicleUpdate = [](int32_t vehicleId, vcmpVehicleUpdate updateType) {
#ifdef DEBUG_ENABLED
		std::cout << "OnVehicleUpdate" << std::endl;
#endif
		try {
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onVehicleUpdate");
			if (fn.isFunction())
				fn(*vehicle);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onVehicleUpdate");
		}
	};
	g_Calls->OnVehicleExplode = [](int32_t vehicleId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnVehicleExplode" << std::endl;
#endif
		try {
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onVehicleExplode");
			if (fn.isFunction())
				fn(*vehicle);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onVehicleExplode");
		}
	};
	g_Calls->OnVehicleRespawn = [](int32_t vehicleId) {
#ifdef DEBUG_ENABLED
		std::cout << "OnVehicleRespawn" << std::endl;
#endif
		try {
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			luabridge::LuaRef fn = luabridge::getGlobal(Lua, "onVehicleRespawn");
			if (fn.isFunction())
				fn(*vehicle);
		}
		catch (luabridge::LuaException e) {
			OutputErrorCallbackCall("onVehicleRespawn");
		}
	};

//	/*** Object ***/
//	g_Calls->OnObjectShot = [](int32_t objectId, int32_t playerId, int32_t weaponId) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnObjectShot" << std::endl;
//#endif
//		lua_getglobal(LUA, "onObjectShot");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) objectId);
//			lua_pushinteger(LUA, (lua_Integer) playerId);
//			lua_pushinteger(LUA, (lua_Integer) weaponId);
//			VerifyLua(LUA, lua_pcall(LUA, 3, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//	g_Calls->OnObjectTouched = [](int32_t objectId, int32_t playerId) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnObjectTouched" << std::endl;
//#endif
//		lua_getglobal(LUA, "onObjectTouched");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) objectId);
//			lua_pushinteger(LUA, (lua_Integer) playerId);
//			VerifyLua(LUA, lua_pcall(LUA, 2, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//
//	/*** Pickup ***/
//
//	g_Calls->OnPickupPickAttempt = [](int32_t pickupId, int32_t playerId) -> uint8_t {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnPickupPickAttempt" << std::endl;
//#endif
//		lua_getglobal(LUA, "onPickupPickAttempt");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) pickupId);
//			lua_pushinteger(LUA, (lua_Integer) playerId);
//			if (VerifyLua(LUA, lua_pcall(LUA, 2, 0, 0))) {
//				lua_Integer returnValue = Lua::GetInteger();
//				return (returnValue == NULL ? 1 : (uint8_t) returnValue);
//			}
//		}
//		else { lua_pop(LUA, -1); }
//		return (uint8_t) 1;
//	};
//	g_Calls->OnPickupPicked = [](int32_t pickupId, int32_t playerId) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnPickupPicked" << std::endl;
//#endif
//		lua_getglobal(LUA, "onPickupPicked");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) pickupId);
//			lua_pushinteger(LUA, (lua_Integer) playerId);
//			VerifyLua(LUA, lua_pcall(LUA, 2, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//	g_Calls->OnPickupRespawn = [](int32_t pickupId) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnPickupRespawn" << std::endl;
//#endif
//		lua_getglobal(LUA, "onPickupRespawn");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) pickupId);
//			VerifyLua(LUA, lua_pcall(LUA, 1, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//
//	/*** Checkpoint ***/
//
//	g_Calls->OnCheckpointEntered = [](int32_t checkPointId, int32_t playerId) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnCheckpointEntered" << std::endl;
//#endif
//		lua_getglobal(LUA, "onCheckpointEntered");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) checkPointId);
//			lua_pushinteger(LUA, (lua_Integer) playerId);
//			VerifyLua(LUA, lua_pcall(LUA, 2, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//	g_Calls->OnCheckpointExited = [](int32_t checkPointId, int32_t playerId) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnCheckpointExited" << std::endl;
//#endif
//		lua_getglobal(LUA, "onCheckpointExited");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) checkPointId);
//			lua_pushinteger(LUA, (lua_Integer) playerId);
//			VerifyLua(LUA, lua_pcall(LUA, 2, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//
//	/*** Misc ***/
//
//	g_Calls->OnEntityPoolChange = [](vcmpEntityPool entityType, int32_t entityId, uint8_t isDeleted) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnEntityPoolChange" << std::endl;
//#endif
//		lua_getglobal(LUA, "onEntityPoolChange");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) entityType);
//			lua_pushinteger(LUA, (lua_Integer) entityId);
//			lua_pushinteger(LUA, (lua_Integer) isDeleted);
//			VerifyLua(LUA, lua_pcall(LUA, 3, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
//	g_Calls->OnServerPerformanceReport = [](size_t entryCount, const char** descriptions, uint64_t* times) {
//#ifdef DEBUG_ENABLED
//		std::cout << "OnServerPerformanceReport" << std::endl;
//#endif
//		lua_getglobal(LUA, "onServerPerformanceReport");
//		if (lua_isfunction(LUA, -1)) {
//			lua_pushinteger(LUA, (lua_Integer) entryCount);
//			lua_pushstring(LUA, *descriptions);
//			lua_pushinteger(LUA, (lua_Integer) entryCount);
//			VerifyLua(LUA, lua_pcall(LUA, 3, 0, 0));
//		}
//		else { lua_pop(LUA, -1); }
//	};
}