#pragma once
#include "pch.h"

typedef std::unordered_map<std::string, vcmpServerOption> optionUMType;

class Server {
public:
	static void Init(lua_State*);

private:
	static bool getOption(const std::string& option);
	static bool setOption(const std::string& option, bool toggle);
	static luabridge::LuaRef getSettings();

public:
	static optionUMType s_OptionCode;
};