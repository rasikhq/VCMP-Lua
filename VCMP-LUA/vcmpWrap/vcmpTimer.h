#pragma once
#include "pch.h"

typedef luabridge::LuaRef luaObject;

class vcmpTimer {
public:
	static void Init(lua_State* Lua);

	vcmpTimer(luaObject callback, unsigned int interval, int32_t repeat, luaObject args);
	inline unsigned int GetInterval() { return m_interval; }
	inline int32_t GetRepeat() { return m_repeat; }
	inline int64_t GetLastTick() { return m_LastTick; }
	inline luaObject& GetCallback() { return m_callback; }
	inline const luaObject GetArgs() { return m_args; }

	inline void SetRepeat(int32_t value) { m_repeat = value; }
	inline void SetLastTick(int64_t value) { m_LastTick = value; }

	bool bIsValid = false;
private:
	luaObject m_callback;
	unsigned int m_interval;
	int32_t m_repeat;
	luaObject m_args;

	int64_t m_LastTick = 0;
};