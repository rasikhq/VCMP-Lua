#pragma once
#include "vcmpTimer.h"

class TimerManager {
public:
	static void Init(sol::state* Lua);
	static void OnFrame(float elapsedTime);

	static vcmpTimer* createTimer(sol::protected_function callback, unsigned int interval, int32_t repeat, sol::table args);
	static void destroyTimer(vcmpTimer* reference);
private:
	static std::vector<vcmpTimer*> m_vcmpTimers;
};