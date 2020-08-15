#pragma once
#include "vcmpTimer.h"
#include "../Constants.h"

class TimerManager {
public:
	static void Init(sol::state* Lua);
	static void OnFrame(float elapsedTime);

	static vcmpTimer* createTimer(sol::function callback, unsigned int interval, int32_t repeat, sol::variadic_args args);
	static void destroyTimer(vcmpTimer* reference);
private:
	static std::vector<std::unique_ptr<vcmpTimer>> m_vcmpTimers;
};