#pragma once
#include "pch.h"
#include "utility.h"

class EventManager {
public:
	static void Init(sol::state* Lua);
	static const std::vector<sol::function>& GetHandlers(std::string eventName);
	static void cancelEvent() { m_bWasEventCancelled = false; }

	static std::unordered_map<std::string, std::vector<sol::function>> m_Handlers;
	static bool m_bWasEventCancelled;

	/*** METHODS ***/
	static bool bind(std::string eventName, sol::function handler);
	static bool unbind(std::string eventName, sol::function handler);
	static bool cancel();
};