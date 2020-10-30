#pragma once
#include "utility.h"
#include "EventManager/EventManager.h"
#include "Timer/TimerManager.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

extern PluginFuncs* g_Funcs;
extern PluginCallbacks* g_Calls;
extern sol::state Lua;

void RegisterVCMPCallbacks() {
	/*** SERVER ***/
	g_Calls->OnServerInitialise = [] () -> uint8_t {
		spdlog::debug("onServerinitialise");
		
		 auto handlers = EventManager::GetHandlers("onServerInit");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn();
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnServerShutdown = []() -> void {
		spdlog::debug("OnServerShutdown");

		auto handlers = EventManager::GetHandlers("onServerShutdown");
		if (handlers.size() == 0) return;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn();
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnServerFrame = [](float elapsedTime) -> void {
		//spdlog::debug("OnServerFrame");

		TimerManager::OnFrame(elapsedTime);
		auto handlers = EventManager::GetHandlers("onServerFrame");
		if (handlers.size() == 0) return;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(elapsedTime);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};
	
	g_Calls->OnPluginCommand = [](uint32_t commandIdentifier, const char* message) -> uint8_t {
		spdlog::debug("OnPluginCommand");

		auto handlers = EventManager::GetHandlers("onPluginCommand");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(commandIdentifier, message);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	/*** PLAYER ***/
	g_Calls->OnIncomingConnection = [](char* playerName, size_t nameBufferSize, const char* userPassword, const char* ipAddress) -> uint8_t {
		spdlog::debug("OnIncomingConnection");

		auto handlers = EventManager::GetHandlers("onPlayerConnection");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(playerName, nameBufferSize, userPassword, ipAddress);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnClientScriptData = [](int32_t playerId, const uint8_t* data, size_t size) {
		spdlog::debug("OnClientScriptData");

		auto handlers = EventManager::GetHandlers("onClientData");
		if (handlers.size() == 0) return;
		Stream stream(data, size);
		Player* player = Player::Get(playerId);
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player, stream, size);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
			}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
		}
	}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};
	
	g_Calls->OnPlayerConnect = [](int32_t playerId) {
		spdlog::debug("OnPlayerConnect");
		auto handlers = EventManager::GetHandlers("onPlayerConnect");

		Player* player = Player::Register(playerId);
		if (handlers.size() == 0) return;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};
	
	g_Calls->OnPlayerDisconnect = [](int32_t playerId, vcmpDisconnectReason reason) {
		spdlog::debug("OnPlayerDisconnect");
		auto handlers = EventManager::GetHandlers("onPlayerDisconnect");
		
		Player* player = Player::Get(playerId);
		Player::Unregister(player);

		if (handlers.size() == 0) return;

		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player, reason);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerModuleList = [](int32_t playerId, const char* list) {
		spdlog::debug("OnPlayerModuleList");

		auto handlers = EventManager::GetHandlers("onPlayerModuleList");
		if (handlers.size() == 0) return;

		Player* player = Player::Get(playerId);
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player, list);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerRequestClass = [](int32_t playerId, int32_t offset) -> uint8_t {
		spdlog::debug("OnPlayerRequestClass");

		auto handlers = EventManager::GetHandlers("onPlayerRequestClass");
		if (handlers.size() == 0) return 1;
		Player* player = Player::Get(playerId);
		uint8_t ret = 1;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player, offset);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnPlayerRequestSpawn = [](int32_t playerId) -> uint8_t {
		spdlog::debug("OnPlayerRequestSpawn");

		auto handlers = EventManager::GetHandlers("onPlayerRequestSpawn");
		if (handlers.size() == 0) return 1;
		Player* player = Player::Get(playerId);
		uint8_t ret = 1;
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnPlayerSpawn = [](int32_t playerId) {
		spdlog::debug("OnPlayerSpawn");

		auto handlers = EventManager::GetHandlers("onPlayerSpawn");
		if (handlers.size() == 0) return;
		Player* player = Player::Get(playerId);
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerDeath = [](int32_t playerId, int32_t killerId, int32_t reason, vcmpBodyPart bodyPart) {
		spdlog::debug("OnPlayerDeath");

		if (!g_Funcs->IsPlayerConnected(killerId)) {
			auto handlers = EventManager::GetHandlers("onPlayerWasted");
			if (handlers.size() > 0) {
				try {
					Player* player = Player::Get(playerId);
					for (auto fn : handlers) {
						if (reason == 43 || reason == 50)
							reason = 43; // drowned
						else if (reason == 39 && bodyPart == 7)
							reason = 39; // car crash
						else if (reason == 39 || reason == 40 || reason == 44)
							reason = 44; // fell

						sol::function_result r = fn(player, reason);
						if (!r.valid()) {
							sol::error e = r;
							spdlog::error("Event callback error: {}", e.what());
						}
						if (EventManager::m_bWasEventCancelled) {
							EventManager::cancelEvent();
							break;
						}
						return;
					}
				}
				catch (sol::error e) {
					spdlog::error(e.what());
				}
			}
		}

		auto handlers = EventManager::GetHandlers("onPlayerKill");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			Player* killer = Player::Get(killerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(killer, player, reason, bodyPart);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerUpdate = [](int32_t playerId, vcmpPlayerUpdate updateType) {
		//spdlog::debug("OnPlayerUpdate");

		auto handlers = EventManager::GetHandlers("onPlayerUpdate");
		if (handlers.size() == 0) return;
		Player* player = Player::Get(playerId);
		try {
			for (auto fn : handlers) {
				sol::function_result r = fn(player, updateType);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerRequestEnterVehicle = [](int32_t playerId, int32_t vehicleId, int32_t slotIndex) -> uint8_t {
		spdlog::debug("OnPlayerRequestEnterVehicle");

		auto handlers = EventManager::GetHandlers("onPlayerRequestEnterVehicle");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, vehicle, vehicleId, slotIndex);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnPlayerEnterVehicle = [](int32_t playerId, int32_t vehicleId, int32_t slotIndex) {
		spdlog::debug("OnPlayerEnterVehicle");

		auto handlers = EventManager::GetHandlers("onPlayerEnterVehicle");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, vehicle, slotIndex);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerExitVehicle = [](int32_t playerId, int32_t vehicleId) {
		spdlog::debug("OnPlayerExitVehicle");

		auto handlers = EventManager::GetHandlers("onPlayerExitVehicle");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, vehicle);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerNameChange = [](int32_t playerId, const char* oldName, const char* newName) {
		spdlog::debug("OnPlayerNameChange");

		auto handlers = EventManager::GetHandlers("onPlayerNameChange");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, oldName, newName);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerStateChange = [](int32_t playerId, vcmpPlayerState oldState, vcmpPlayerState newState) {
		//spdlog::debug("OnPlayerStateChange");

		auto handlers = EventManager::GetHandlers("onPlayerStateChange");
		if (handlers.size() == 0 || newState == oldState) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, oldState, newState);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerActionChange = [](int32_t playerId, int32_t oldAction, int32_t newAction) {
		spdlog::debug("OnPlayerActionChange");

		auto handlers = EventManager::GetHandlers("onPlayerActionChange");
		if (handlers.size() == 0 || newAction == oldAction) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, oldAction, newAction);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerOnFireChange = [](int32_t playerId, uint8_t isOnFire) {
		spdlog::debug("OnPlayerOnFireChange");

		auto handlers = EventManager::GetHandlers("onPlayerFireChange");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, isOnFire);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerCrouchChange = [](int32_t playerId, uint8_t isCrouching) {
		spdlog::debug("OnPlayerCrouchChange");

		auto handlers = EventManager::GetHandlers("onPlayerCrouchChange");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, isCrouching);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerGameKeysChange = [](int32_t playerId, uint32_t oldKeys, uint32_t newKeys) {
		//spdlog::debug("OnPlayerGameKeysChange");

		auto handlers = EventManager::GetHandlers("onPlayerGameKeysChange");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, oldKeys, newKeys);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerBeginTyping = [](int32_t playerId) {
		spdlog::debug("OnPlayerBeginTyping");

		auto handlers = EventManager::GetHandlers("onPlayerBeginTyping");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerEndTyping = [](int32_t playerId) {
		spdlog::debug("OnPlayerEndTyping");

		auto handlers = EventManager::GetHandlers("onPlayerFinishTyping");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerAwayChange = [](int32_t playerId, uint8_t isAway) {
		spdlog::debug("OnPlayerAwayChange");

		auto handlers = EventManager::GetHandlers("onPlayerAwayChange");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, isAway);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerMessage = [](int32_t playerId, const char* message) -> uint8_t {
		spdlog::debug("OnPlayerMessage");

		auto handlers = EventManager::GetHandlers("onPlayerMessage");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, message);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnPlayerCommand = [](int32_t playerId, const char* message) -> uint8_t {
		spdlog::debug("OnPlayerCommand");

		auto handlers = EventManager::GetHandlers("onPlayerCommand");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			if (player == nullptr) {
				spdlog::critical("VCMP CALLBACK :: OnPlayerCommand >> Got a playerid ({}) which is not reigstered in player pool", playerId);
				throw("Critical failure");
				return 0;
			}
			std::string data(message);
			std::vector<std::string> args = std::split(data, ' ');
			for (auto fn : handlers) {
				if (args.size() <= 0) { // No command at all? Pass nil
					sol::function_result r = fn(player, sol::lua_nil, sol::lua_nil);
					if (!r.valid()) {
						sol::error e = r;
						spdlog::error("Event callback error: {}", e.what());
					}
					if (EventManager::m_bWasEventCancelled) {
						EventManager::cancelEvent();
						ret = 0;
						break;
					}
				}
				else {
					std::string command = args.at(0);
					args.erase(args.begin());
					if (args.size() > 0) {
						sol::function_result r = fn(player, command, sol::as_table_t<std::vector<std::string>>(args));
						if (!r.valid()) {
							sol::error e = r;
							spdlog::error("Event callback error: {}", e.what());
						}
						if (EventManager::m_bWasEventCancelled) {
							EventManager::cancelEvent();
							ret = 0;
							break;
						}
					}
					else {
						sol::function_result r = fn(player, command, sol::lua_nil);
						if (!r.valid()) {
							sol::error e = r;
							spdlog::error("Event callback error: {}", e.what());
						}
						if (EventManager::m_bWasEventCancelled) {
							EventManager::cancelEvent();
							ret = 0;
							break;
						}
					}
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnPlayerPrivateMessage = [](int32_t playerId, int32_t targetPlayerId, const char* message) -> uint8_t {
		spdlog::debug("OnPlayerPrivateMessage");

		auto handlers = EventManager::GetHandlers("onPlayerPM");
		if (handlers.size() == 0) return 1;
		uint8_t ret = 1;
		try {
			Player* player = Player::Get(playerId);
			Player* targetPlayer = Player::Get(targetPlayerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, targetPlayer, message);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					ret = 0;
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
		return ret;
	};

	g_Calls->OnPlayerKeyBindDown = [](int32_t playerId, int32_t bindId) {
		spdlog::debug("OnPlayerKeyBindDown");

		auto handlers = EventManager::GetHandlers("onPlayerKeyDown");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, bindId);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerKeyBindUp = [](int32_t playerId, int32_t bindId) {
		spdlog::debug("OnPlayerKeyBindUp");

		auto handlers = EventManager::GetHandlers("onPlayerKeyUp");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, bindId);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerSpectate = [](int32_t playerId, int32_t targetPlayerId) {
		spdlog::debug("OnPlayerSpectate");

		auto handlers = EventManager::GetHandlers("onPlayerSpectate");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			Player* targetPlayer = Player::Get(targetPlayerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, targetPlayer);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnPlayerCrashReport = [](int32_t playerId, const char* report) {
		spdlog::debug("OnPlayerCrashReport");

		auto handlers = EventManager::GetHandlers("onPlayerCrashReport");
		if (handlers.size() == 0) return;
		try {
			Player* player = Player::Get(playerId);
			for (auto fn : handlers) {
				sol::function_result r = fn(player, report);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};
	
	/*** VEHICLE ***/
	g_Calls->OnVehicleUpdate = [](int32_t vehicleId, vcmpVehicleUpdate updateType) {
		//spdlog::debug("OnVehicleUpdate");

		auto handlers = EventManager::GetHandlers("onVehicleUpdate");
		if (handlers.size() == 0) return;
		try {
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			for (auto fn : handlers) {
				sol::function_result r = fn(vehicle, updateType);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnVehicleExplode = [](int32_t vehicleId) {
		spdlog::debug("OnVehicleExplode");

		auto handlers = EventManager::GetHandlers("onVehicleExplode");
		if (handlers.size() == 0) return;
		try {
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			for (auto fn : handlers) {
				sol::function_result r = fn(vehicle);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	g_Calls->OnVehicleRespawn = [](int32_t vehicleId) {
		spdlog::debug("OnVehicleRespawn");

		auto handlers = EventManager::GetHandlers("onVehicleRespawn");
		if (handlers.size() == 0) return;
		try {
			Vehicle* vehicle = Vehicle::Get(vehicleId);
			for (auto fn : handlers) {
				sol::function_result r = fn(vehicle);
				if (!r.valid()) {
					sol::error e = r;
					spdlog::error("Event callback error: {}", e.what());
				}
				if (EventManager::m_bWasEventCancelled) {
					EventManager::cancelEvent();
					break;
				}
			}
		}
		catch (sol::error e) {
			spdlog::error(e.what());
		}
	};

	/*** OBJECT ***/
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
	/*** PICKUP ***/
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
	/*** CHECKPOINT ***/
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
	/*** MISC ***/
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