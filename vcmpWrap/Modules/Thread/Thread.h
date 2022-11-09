#pragma once
#include <pch.h>
#include <async++.h>

using LuaTypes = std::variant<sol::lua_nil_t, int, float, std::string>;

class Thread
{
public:
	static void Init(sol::state* Lua);

public:
	Thread();
	~Thread();

private:
	void run(sol::function f);
	bool wait();
	sol::object get();
	bool isReady();
	bool isCancelled();
	void destroy();

private:
	async::cancellation_token c;
	async::task<sol::object> task;
};