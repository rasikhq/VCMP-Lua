#pragma once
#include "vcmpTimer.h"

class TimerManager {
public:
	static void Init(lua_State* Lua);
	static void onFrame(float elapsedTime);

	static vcmpTimer* CreateTimer(luaObject callback, unsigned int interval, int32_t repeat, luaObject args);
	static void DestroyTimer(vcmpTimer* reference);
private:
	static std::vector<vcmpTimer*> m_vcmpTimers;
};