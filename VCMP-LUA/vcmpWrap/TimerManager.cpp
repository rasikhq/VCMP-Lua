#include "TimerManager.h"
#include "utility.h"

extern sol::state Lua;
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
				sol::protected_function& fn = timer->GetCallback();
				if (fn.valid()) {
					sol::table args = timer->GetArgs();
					sol::protected_function_result result = fn(args);
					if (!result.valid()) {
						sol::error e = result;
						OutputError("Timer handler failed: %s", e.what());
					}
				}
				else {
					timer->bIsValid = false;
					continue;
				}
			}
			catch (sol::error e) {
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

vcmpTimer* TimerManager::CreateTimer(sol::protected_function callback, unsigned int interval, int32_t repeat, sol::table args) {
	if (!callback.valid()) {
		OutputError("Expected function at argument 1");
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

void TimerManager::Init(sol::state* Lua) {
	m_vcmpTimers.reserve(256);

	sol::usertype<TimerManager> userdata = Lua->new_usertype<TimerManager>("Timer");

	userdata["create"] = &TimerManager::CreateTimer;
	userdata["destroy"] = &TimerManager::DestroyTimer;
}