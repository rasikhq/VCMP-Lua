#pragma once
#include "pch.h"

class EventManager {
public:
	static void Init(sol::state* Lua);
	static void Reset();
	static void Trigger(const std::string& eventName);
	static const std::vector<sol::function>& GetHandlers(std::string eventName);
	static bool eventExists(const std::string& eventName);
	static inline void cancelEvent() { m_bWasEventCancelled = false; }

	static std::unordered_map<std::string, std::vector<sol::function>> m_Handlers;
	static bool m_bWasEventCancelled;

	/*** METHODS ***/
	static bool create(std::string eventName);
	static bool trigger(std::string eventName, sol::variadic_args args);

	static bool bind(std::string eventName, sol::function handler);
	static bool unbind(std::string eventName, sol::function handler);
	static bool cancel();
};