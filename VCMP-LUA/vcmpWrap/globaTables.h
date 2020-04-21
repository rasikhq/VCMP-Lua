#pragma once
#include "TimerManager.h"
#include "Classes/Server.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

void RegisterClasses(lua_State* Lua) {
	TimerManager::Init(Lua);
	Server::Init(Lua);
	Player::init(Lua);
	Vehicle::init(Lua);
}