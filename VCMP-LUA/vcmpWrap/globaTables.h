#pragma once
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

void RegisterClasses(lua_State* Lua) {
	Player::init(Lua);
	Vehicle::init(Lua);
}