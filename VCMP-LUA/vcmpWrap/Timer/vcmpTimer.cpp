#include "vcmpTimer.h"

vcmpTimer::vcmpTimer(sol::function callback, unsigned int interval, int32_t repeat, std::vector<sol::object> args)
	: m_callback(callback), m_interval(interval), m_repeat(repeat), m_args(args)
{
	setLastTick(GetCurrentSysTime());
	this->bIsValid = true;
}

void vcmpTimer::Init(sol::state* Lua) {}