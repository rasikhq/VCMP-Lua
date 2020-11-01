#pragma once
#include "pch.h"

class Server {
public:
	static void Init(sol::state*);

private:
	static bool getOption(vcmpServerOption option);
	static bool setOption(vcmpServerOption option, bool toggle);
	static sol::object getSettings();
};