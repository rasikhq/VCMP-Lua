#pragma once
#include "TimerManager.h"
#include "Classes/Server.h"
#include "Classes/Player.h"
#include "Classes/Vehicle.h"

void RegisterClasses(sol::state* Lua) {
	sol::state& state = *Lua;

	TimerManager::Init(Lua);
	Server::Init(Lua);
	Player::Init(Lua);
	Vehicle::Init(Lua);
}