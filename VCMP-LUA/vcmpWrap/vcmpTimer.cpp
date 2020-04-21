#include "vcmpTimer.h"

vcmpTimer::vcmpTimer(luaObject callback, unsigned int interval, int32_t repeat, luaObject args)
	: m_callback(callback), m_interval(interval), m_repeat(repeat), m_args(args)
{
	this->bIsValid = true;
}

void vcmpTimer::Init(lua_State* Lua) {
	luabridge::getGlobalNamespace(Lua)
		.beginClass<vcmpTimer>("__vcmpTimerBase__")
		.endClass();
}