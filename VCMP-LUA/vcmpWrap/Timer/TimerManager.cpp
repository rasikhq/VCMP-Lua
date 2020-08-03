#include "TimerManager.h"
#include "utility.h"

extern sol::state Lua;
std::vector<vcmpTimer*> TimerManager::m_vcmpTimers;

#define DEBUG_TIMERMANAGER 1
#undef DEBUG_TIMERMANAGER

void TimerManager::OnFrame(float elapsedTime) {
	for (auto i = 0; i < m_vcmpTimers.size(); i++) {
		vcmpTimer* timer = m_vcmpTimers[i];
		if (timer->getLastTick() == 0) {
			timer->setLastTick(GetCurrentSysTime());
			continue;
		}
		else if (!timer->bIsValid) {
			delete m_vcmpTimers[i];
			m_vcmpTimers.erase(m_vcmpTimers.begin() + i);
			continue;
		}

		int64_t currentTick = GetCurrentSysTime();
		int64_t lastTick = timer->getLastTick();

		int32_t repeat = timer->getRepeat();

		// Calculate the elapsed time
		const auto delta = int64_t((currentTick - lastTick) / 1000L);

		if (delta > timer->getInterval() && repeat != 0) {
			try {
				const sol::function& fn = timer->getCallback();
				if (fn.valid()) {
					const std::vector<sol::object>& args = timer->getArgs();
#ifdef DEBUG_TIMERMANAGER
					
#endif
					sol::function_result result = fn(sol::as_args(args));
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
			timer->setLastTick(currentTick);
			if (repeat > 0) {
				repeat--;
				timer->setRepeat(repeat);

				if (repeat == 0) {
					timer->bIsValid = false;
				}
			}
		}
	}
}

vcmpTimer* TimerManager::createTimer(sol::function callback, unsigned int interval, int32_t repeat, sol::variadic_args args) {
	if (!callback.valid()) {
		OutputError("Expected function at argument 1");
		return nullptr;
	}
	else if (interval < 50) {
		OutputError("Interval cannot be less tahn 50ms!");
		return nullptr;
	}
	std::vector<sol::object> largs(args.begin(), args.end());
#ifdef DEBUG_TIMERMANAGER
	std::cout << "createTimer :: Received VA of size: " << args.size() << std::endl;
#endif
	m_vcmpTimers.push_back(new vcmpTimer(callback, interval, repeat, largs));
	return m_vcmpTimers.back();
}

void TimerManager::destroyTimer(vcmpTimer* reference) {
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

	userdata["create"] = &TimerManager::createTimer;
	userdata["destroy"] = &TimerManager::destroyTimer;
}