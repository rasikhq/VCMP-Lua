#pragma once
#include "pch.h"

class vcmpTimer {
public:
	static void Init(sol::state* Lua);

	vcmpTimer(sol::protected_function callback, unsigned int interval, int32_t repeat, sol::table args);
	inline unsigned int GetInterval() { return m_interval; }
	inline int32_t GetRepeat() { return m_repeat; }
	inline int64_t GetLastTick() { return m_LastTick; }
	inline sol::protected_function& GetCallback() { return m_callback; }
	inline const sol::table& GetArgs() { return m_args; }

	inline void SetRepeat(int32_t value) { m_repeat = value; }
	inline void SetLastTick(int64_t value) { m_LastTick = value; }

	bool bIsValid = false;
private:
	sol::protected_function m_callback;
	unsigned int m_interval;
	int32_t m_repeat;
	sol::table m_args;

	int64_t m_LastTick = 0;
};