#pragma once
#include "pch.h"

class vcmpTimer {
public:
	static void Init(sol::state* Lua);

	vcmpTimer(sol::protected_function callback, unsigned int interval, int32_t repeat, sol::table args);
	inline unsigned int getInterval() { return m_interval; }
	inline int32_t getRepeat() { return m_repeat; }
	inline int64_t getLastTick() { return m_LastTick; }
	inline sol::protected_function& getCallback() { return m_callback; }
	inline const sol::table& getArgs() { return m_args; }

	inline void setRepeat(int32_t value) { m_repeat = value; }
	inline void setLastTick(int64_t value) { m_LastTick = value; }

	bool bIsValid = false;
private:
	sol::protected_function m_callback;
	unsigned int m_interval;
	int32_t m_repeat;
	sol::table m_args;

	int64_t m_LastTick = 0;
};