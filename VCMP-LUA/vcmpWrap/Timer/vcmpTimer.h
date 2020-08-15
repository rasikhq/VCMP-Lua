#pragma once
#include "pch.h"
#include "utility.h"

class vcmpTimer {
public:
	static void Init(sol::state* Lua);

	vcmpTimer(sol::function callback, unsigned int interval, int32_t repeat, std::vector<sol::object> args);
	~vcmpTimer() { bIsValid = false; }

	inline unsigned int getInterval() { return m_interval; }
	inline int32_t getRepeat() { return m_repeat; }
	inline int64_t getLastTick() { return m_LastTick; }
	inline const sol::function& getCallback() { return m_callback; }
	inline const std::vector<sol::object>& getArgs() { return m_args; }

	inline void setRepeat(int32_t value) { m_repeat = value; }
	inline void setLastTick(int64_t value) { m_LastTick = value; }

	bool bIsValid = false;
private:
	sol::function m_callback;
	unsigned int m_interval;
	int32_t m_repeat;
	std::vector<sol::object> m_args;

	int64_t m_LastTick = 0;
};