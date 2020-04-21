#include "TimerManager.h"
#include "utility.h"

extern lua_State* Lua;
std::vector<vcmpTimer*> TimerManager::m_vcmpTimers;

void TimerManager::onFrame(float elapsedTime) {
	for (auto i = 0; i < m_vcmpTimers.size(); i++) {
		vcmpTimer* timer = m_vcmpTimers[i];
		if (timer->GetLastTick() == 0) {
			timer->SetLastTick(GetCurrentSysTime());
			continue;
		}
		else if (!timer->bIsValid) {
			delete m_vcmpTimers[i];
			m_vcmpTimers.erase(m_vcmpTimers.begin() + i);
			continue;
		}

		int64_t currentTick = GetCurrentSysTime();
		int64_t lastTick = timer->GetLastTick();

		int32_t repeat = timer->GetRepeat();

		// Calculate the elapsed time
		const auto delta = int64_t((currentTick - lastTick) / 1000L);

		if (delta > timer->GetInterval() && repeat != 0) {
			try {
				luabridge::LuaRef fn = timer->GetCallback();
				if (fn.isFunction()) {
					luabridge::LuaRef args = timer->GetArgs();
					fn(args);
				}
				else {
					timer->bIsValid = false;
					continue;
				}
			}
			catch (luabridge::LuaException e) {
				OutputError(e.what());
			}
			timer->SetLastTick(currentTick);
			if (repeat > 0) {
				repeat--;
				timer->SetRepeat(repeat);

				if (repeat == 0) {
					timer->bIsValid = false;
				}
			}
		}
	}
}

vcmpTimer* TimerManager::CreateTimer(luaObject callback, unsigned int interval, int32_t repeat, luaObject args) {
	if (!callback.isFunction()) {
		OutputError("Expected function at argument 1");
		return luabridge::LuaRef(Lua);
	}
	m_vcmpTimers.push_back(new vcmpTimer(callback, interval, repeat, args));
	return m_vcmpTimers.back();
}

void TimerManager::DestroyTimer(vcmpTimer* reference) {
	for (auto it = m_vcmpTimers.begin(); it != m_vcmpTimers.end(); it++) {
		if ((*it) == reference) {
			reference->bIsValid = false;
			break;
		}
	}
}

void TimerManager::Init(lua_State* Lua) {
	m_vcmpTimers.reserve(256);

	vcmpTimer::Init(Lua);

	luabridge::getGlobalNamespace(Lua)
		.beginClass<TimerManager>("Timer")

		.addStaticFunction("create", TimerManager::CreateTimer)
		.addStaticFunction("destroy", TimerManager::DestroyTimer)

		.endClass();
}