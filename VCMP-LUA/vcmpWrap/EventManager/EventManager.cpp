#include "EventManager.h"

std::unordered_map<std::string, std::vector<sol::function>> EventManager::m_Handlers = {
	/*** SERVER **/
	{"onServerInit", {}},
	{"onServerShutdown", {}},
	{"onServerFrame", {}},
	{"onPluginCommand", {}},

	/*** PLAYER ***/
	{"onPlayerConnection", {}},
	{"onPlayerConnect", {}},
	{"onPlayerDisconnect", {}},
	{"onPlayerRequestClass", {}},
	{"onPlayerRequestSpawn", {}},
	{"onPlayerSpawn", {}},
	{"onPlayerWasted", {}},
	{"onPlayerKill", {}},
	{"onPlayerUpdate", {}},
	{"onPlayerRequestEnterVehicle", {}},
	{"onPlayerEnterVehicle", {}},
	{"onPlayerExitVehicle", {}},
	{"onPlayerNameChange", {}},
	{"onPlayerStateChange", {}},
	{"onPlayerActionChange", {}},
	{"onPlayerFireChange", {}},
	{"onPlayerCrouchChange", {}},
	{"onPlayerGameKeysChange", {}},
	{"onPlayerBeginTyping", {}},
	{"onPlayerFinishTyping", {}},
	{"onPlayerAwayChange", {}},
	{"onPlayerMessage", {}},
	{"onPlayerCommand", {}},
	{"onPlayerPM", {}},
	{"onPlayerKeyDown", {}},
	{"onPlayerKeyUp", {}},
	{"onPlayerSpectate", {}},
	{"onPlayerCrashReport", {}},

	/*** VEHICLES ***/
	{"onVehicleUpdate", {}},
	{"onVehicleExplode", {}},
	{"onVehicleRespawn", {}},
};

bool EventManager::m_bWasEventCancelled = false;

void EventManager::Init(sol::state* Lua) {
	sol::usertype<EventManager> userdata = Lua->new_usertype<EventManager>("Event");

	userdata["create"] = &EventManager::create;
	userdata["trigger"] = &EventManager::trigger;

	userdata["bind"] = &EventManager::bind;
	userdata["unbind"] = &EventManager::unbind;
	userdata["cancel"] = &EventManager::cancel;
}

const std::vector<sol::function>& EventManager::GetHandlers(std::string eventName) {
	return m_Handlers[eventName];
}

bool EventManager::eventExists(const std::string& eventName) {
	auto eventExists = m_Handlers.find(eventName);
	if (eventExists != m_Handlers.end())
		return true;
	return false;
}

bool EventManager::create(std::string eventName) {
	if (eventExists(eventName)) {
		OutputError("Event Manager :: A custom event named %s already exists", eventName);
		return false;
	}
	m_Handlers[eventName] = {};
	return true;
}

bool EventManager::trigger(std::string eventName, sol::variadic_args args) {
	if (!eventExists(eventName)) {
		OutputError("Event Manager :: A custom event with the name %s does not exist", eventName);
		return false;
	}

	std::vector<sol::object> largs(args.begin(), args.end());
	for (auto fn : GetHandlers(eventName)) {
		fn(sol::as_args(largs));
		if (EventManager::m_bWasEventCancelled) {
			EventManager::cancelEvent();
			break;
		}
	}
}

bool EventManager::bind(std::string eventName, sol::function handler) {
	if (!eventExists(eventName)) {
		OutputError("Event Manager :: No such event named %s exists", eventName);
		return false;
	}

	for (const auto& handle : m_Handlers[eventName]) {
		if (handle.pointer() == handler.pointer()) {
			OutputError("Event Manager :: Function handler for event %s is already bound to the given function!", eventName);
			return false;
		}
	}
	m_Handlers[eventName].push_back(handler);
	return true;
}

bool EventManager::unbind(std::string eventName, sol::function handler) {
	if (!eventExists(eventName)) {
		OutputError("Event Manager :: No such named %s exists", eventName);
		return false;
	}

	for (auto it = m_Handlers[eventName].begin(); it != m_Handlers[eventName].end(); it++) {
		if (it->pointer() == handler.pointer()) {
			it = m_Handlers[eventName].erase(it);
			return true;
		}
	}
	OutputError("Event Manager :: No function handlers are bound for event %s!", eventName);
	return false;
}

bool EventManager::cancel() {
	if (!m_bWasEventCancelled)
		m_bWasEventCancelled = true;

	return true;
}