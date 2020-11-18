#pragma once
#include "pch.h"

#include <cpr/cpr.h>

class Remote
{
public:
	static void Init(sol::state* Lua);

	static sol::table fetchHTTP(const std::string& url, sol::table params, sol::table auth);
};