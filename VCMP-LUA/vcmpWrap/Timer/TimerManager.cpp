#include "TimerManager.h"

extern sol::state Lua;
std::vector<vcmpTimer> TimerManager::m_vcmpTimers;

#define DEBUG_TIMERMANAGER 1
#undef DEBUG_TIMERMANAGER

void TimerManager::OnFrame(float elapsedTime) 
{
	if(m_vcmpTimers.empty()) return;

	for (auto timer = m_vcmpTimers.begin(); timer != m_vcmpTimers.end(); timer++) 
	{
		const int64_t currentTick = GetCurrentSysTime();

		if (!timer->bIsValid) 
		{
			timer = m_vcmpTimers.erase(timer);
			if (timer == m_vcmpTimers.end())
				break;
		}

		int64_t lastTick = timer->getLastTick();
		int32_t repeat = timer->getRepeat();

		// Calculate the elapsed time
		const auto delta = int64_t((currentTick - lastTick) / 1000L);

		if (delta > timer->getInterval() && repeat != 0) 
		{
			const sol::function& fn = timer->getCallback();
			if (fn.valid()) 
			{
				Lua["thisTimer"] = &(*timer);
				const std::vector<sol::object>& args = timer->getArgs();
				sol::function_result result = fn(sol::as_args(args));
				Lua["thisTimer"] = sol::nil;
				if (!result.valid()) 
				{
					sol::error e = result;
					spdlog::error("Timer handler failed: {}", e.what());
				}
			}
			else 
			{
				timer->bIsValid = false;
				continue;
			}

			timer->setLastTick(currentTick);

			if (repeat > 0)
				timer->setRepeat(--repeat);
			else
				timer->bIsValid = false;
		}
	}
}

vcmpTimer* TimerManager::createTimer(sol::function callback, unsigned int interval, int32_t repeat, sol::variadic_args args) 
{
	if (m_vcmpTimers.size() >= m_vcmpTimers.capacity()) {
		spdlog::error("Timer object limit of {} reached!", MAX_TIMERS);
		throw("Timers limit reached1");
	}
	if (!callback.valid()) {
		spdlog::error("Expected function at argument 1");
		throw("Invalid callback specified!");
	}
	else if (interval < 50) {
		spdlog::error("Interval cannot be less than 50ms!");
		return nullptr;
	}
	std::vector<sol::object> largs(args.begin(), args.end());

#ifdef DEBUG_TIMERMANAGER
	spdlog::debug("createTimer :: Received VA of size: {}", args.size());
#endif

	return &m_vcmpTimers.emplace_back(callback, interval, repeat, largs);
}

void TimerManager::destroyTimer(vcmpTimer* reference) 
{
	reference->bIsValid = false;
}

void TimerManager::Init(sol::state* Lua) 
{
	m_vcmpTimers.reserve(MAX_TIMERS);

	sol::usertype<TimerManager> userdata = Lua->new_usertype<TimerManager>("Timer");

	userdata["create"] = &TimerManager::createTimer;
	userdata["destroy"] = &TimerManager::destroyTimer;
}