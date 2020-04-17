#pragma once
#include "Classes/Server.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

void RegisterClasses(lua_State* Lua) {
	Server::Init(Lua);
	Player::init(Lua);
	Vehicle::init(Lua);
}